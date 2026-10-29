/**
 * @file    sim_hw.c
 * @brief   Kiểm thử tích hợp trên máy tính (Linux): chạy toàn bộ tầng Application với FreeRTOS POSIX port.
 *          Các module phụ thuộc phần cứng được thay bằng bản giả:
 *            - board, button, light_sensor, max7219: biến trong RAM
 *            - i2c: ghi lại bộ đệm OLED (in ra dạng ký tự khi gọi oled_dump)
 *            - uart: in ra màn hình, gọi ngay callback "truyền xong"
 *          Task "Sim" (ưu tiên 5) đóng vai người dùng: đổi cảm biến, nhấn nút, gõ lệnh,
 *          rồi kiểm tra trạng thái trong mailbox qLampView (EXPECT).
 *          Chạy: make sim
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "button.h"
#include "light_sensor.h"
#include "max7219.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "app_ipc.h"
#include "cpu_monitor.h"
#include "hal_timer.h"

/* board */
int g_led_toggles;
void board_init(void) {}
void board_led_toggle(void) { g_led_toggles++; }
void board_led_set(int on) { (void)on; }

/* button + light sensor */
static button_edge_cb_t s_btn_cb; static light_sensor_edge_cb_t s_ls_cb;
volatile int g_btn[2]; volatile int g_dark;
void button_init(button_edge_cb_t cb) { s_btn_cb = cb; }
bool button_is_pressed(button_id_t id) { return g_btn[id]; }
void light_sensor_init(light_sensor_edge_cb_t cb) { s_ls_cb = cb; }
bool light_sensor_is_dark(void) { return g_dark; }

/* max7219 */
int g_mx_intensity = -1, g_mx_shutdown = 1, g_mx_frames;
uint8_t g_mx_rows[8];
void max7219_init(void) {}
void max7219_set_intensity(uint8_t l) { g_mx_intensity = l; }
void max7219_set_shutdown(bool s) { g_mx_shutdown = s; }
void max7219_write_frame(const uint8_t *r) { memcpy(g_mx_rows, r, 8); g_mx_frames++; }

/* i2c -> OLED framebuffer capture */
static uint8_t s_oled[1024]; static int s_oled_pos; int g_oled_updates;
void i2c1_init(void) {}
void i2c1_get_idle_levels(int *scl, int *sda) { *scl = 1; *sda = 1; }
i2c_status_t i2c1_write(uint8_t a, uint8_t prefix, const uint8_t *d, uint16_t len)
{
    (void)a;
    if (prefix == 0x40) { for (int i = 0; i < len; i++) s_oled[(s_oled_pos++) % 1024] = d[i]; if (s_oled_pos % 1024 == 0) g_oled_updates++; }
    else if (len == 1 && d[0] == 0x21) { s_oled_pos = 0; }
    return I2C_OK;
}
static int s_dump_n;
static void oled_dump(void)
{
    s_dump_n++;
    printf("+--------------------------------------------------------------------------------------------------------------------------------+\n");
    for (int y = 0; y < 64; y += 2) {
        putchar('|');
        for (int x = 0; x < 128; x++) {
            int a = s_oled[x + (y / 8) * 128] >> (y & 7) & 1, b = s_oled[x + ((y + 1) / 8) * 128] >> ((y + 1) & 7) & 1;
            fputs(a && b ? "█" : a ? "▀" : b ? "▄" : " ", stdout);
        }
        puts("|");
    }
    printf("+--------------------------------------------------------------------------------------------------------------------------------+\n");
}

/* uart */
static uart_rx_callback_t s_rx; static uart_tx_done_callback_t s_txd;
void uart1_init(uint32_t b, uint8_t p, uart_rx_callback_t rx, uart_tx_done_callback_t txd) { (void)b; (void)p; s_rx = rx; s_txd = txd; }
bool uart1_send_dma(const uint8_t *buf, uint16_t len)
{
    printf("[%6lu ms] ", (unsigned long)xTaskGetTickCount()); fwrite(buf, 1, len, stdout); fflush(stdout);
    s_txd(); return true;
}
void uart1_send_blocking(const char *s) { fputs(s, stdout); }

static void type_cmd(const char *s)
{
    printf("[%6lu ms] >>> %s\n", (unsigned long)xTaskGetTickCount(), s);
    taskENTER_CRITICAL();
    for (; *s; s++) s_rx((uint8_t)*s);
    s_rx('\r'); s_rx('\n');
    taskEXIT_CRITICAL();
    vTaskDelay(pdMS_TO_TICKS(150));
}
static void press(int id, int ms)
{
    g_btn[id] = 1; s_btn_cb((button_id_t)id); vTaskDelay(pdMS_TO_TICKS(5)); s_btn_cb((button_id_t)id); /* bounce */
    vTaskDelay(pdMS_TO_TICKS(ms));
    g_btn[id] = 0; s_btn_cb((button_id_t)id);
    vTaskDelay(pdMS_TO_TICKS(120));
}
static void set_dark(int d) { printf("[%6lu ms] *** cam bien -> %s\n", (unsigned long)xTaskGetTickCount(), d ? "TOI" : "SANG"); g_dark = d; s_ls_cb(); }

static int s_fail;
#define EXPECT(c) do { if (!(c)) { s_fail++; printf("!!! EXPECT FAIL line %d: %s\n", __LINE__, #c); } else printf("    ok: %s\n", #c); } while (0)
static lamp_view_t view(void) { lamp_view_t v; memset(&v, 0, sizeof v); xQueuePeek(g_qLampView, &v, 0); return v; }

static void sim_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(1300));                 /* qua hiệu chuẩn */
    type_cmd("help");
    type_cmd("status");
    EXPECT(view().state == LAMP_OFF);

    /* nhiễu cảm biến ngắn 300 ms: không được bật đèn */
    set_dark(1); vTaskDelay(pdMS_TO_TICKS(300)); set_dark(0); vTaskDelay(pdMS_TO_TICKS(1500));
    EXPECT(view().state == LAMP_OFF);

    set_dark(1); vTaskDelay(pdMS_TO_TICKS(1500));
    EXPECT(view().state == LAMP_FADE_IN);
    vTaskDelay(pdMS_TO_TICKS(1800));
    EXPECT(view().state == LAMP_ON && view().level == 16);
    EXPECT(g_mx_shutdown == 0 && g_mx_intensity == 15 && g_mx_rows[0] == 0xFF);
    type_cmd("status");
    vTaskDelay(pdMS_TO_TICKS(300));
    oled_dump();

    /* menu: NEXT mở, OK sửa Che do, NEXT -> BREATH, OK lưu */
    press(BUTTON_NEXT, 50);
    EXPECT(view().menu.state == MENU_NAV);
    press(BUTTON_OK, 50); press(BUTTON_NEXT, 50);
    vTaskDelay(pdMS_TO_TICKS(300)); oled_dump();
    press(BUTTON_OK, 50);
    EXPECT(view().settings.mode == LAMP_MODE_BREATH);
    press(BUTTON_NEXT, 1000);                       /* giữ -> đóng menu */
    EXPECT(view().menu.state == MENU_CLOSED);
    vTaskDelay(pdMS_TO_TICKS(1300));
    printf("    breath intensity hien tai = %d\n", g_mx_intensity);

    type_cmd("mode 2"); vTaskDelay(pdMS_TO_TICKS(200));
    uint8_t r1[8]; memcpy(r1, g_mx_rows, 8); vTaskDelay(pdMS_TO_TICKS(300));
    EXPECT(view().settings.mode == LAMP_MODE_WAVE && memcmp(r1, g_mx_rows, 8) != 0);
    type_cmd("bright 4"); vTaskDelay(pdMS_TO_TICKS(100));
    EXPECT(view().level == 4);
    type_cmd("cpu");
    vTaskDelay(pdMS_TO_TICKS(400));

    /* OK ngắn khi menu đóng = tắt đèn */
    press(BUTTON_OK, 50); vTaskDelay(pdMS_TO_TICKS(700));
    EXPECT(view().state == LAMP_OFF && g_mx_shutdown == 1);
    type_cmd("on"); vTaskDelay(pdMS_TO_TICKS(700));
    EXPECT(view().state == LAMP_ON);

    set_dark(0); vTaskDelay(pdMS_TO_TICKS(1700));
    EXPECT(view().state == LAMP_OFF);
    type_cmd("auto 0"); set_dark(1); vTaskDelay(pdMS_TO_TICKS(1500));
    EXPECT(view().state == LAMP_OFF && view().dark);
    type_cmd("auto 1"); vTaskDelay(pdMS_TO_TICKS(300));
    EXPECT(view().state == LAMP_FADE_IN || view().state == LAMP_ON);

    type_cmd("log 1"); vTaskDelay(pdMS_TO_TICKS(2100)); type_cmd("log 0"); oled_dump();
    type_cmd("sleep 1"); type_cmd("xyz"); type_cmd("bright 99");
    cpu_stats_t st; cpu_monitor_get(&st, 10);
    EXPECT(st.task_count == 9 && st.idle_max > 0);
    printf("    LED toggles = %d, OLED updates = %d, matrix frames = %d\n", g_led_toggles, g_oled_updates, g_mx_frames);
    EXPECT(g_led_toggles >= 10);
    printf("\n=== SIM KET THUC: %d loi ===\n", s_fail);
    fflush(stdout);
    exit(s_fail ? 1 : 0);
}

void sim_start(void) { xTaskCreate(sim_task, "Sim", 4096, NULL, 5, NULL); }
void app_assert_failed(const char *f, int l) { printf("ASSERT %s:%d\n", f, l); fflush(stdout); abort(); }
void vApplicationStackOverflowHook(TaskHandle_t t, char *n) { printf("STACK OVF %s\n", n); abort(); }
void vApplicationMallocFailedHook(void) { printf("MALLOC FAIL\n"); abort(); }

/* Thang thời gian 20 kHz cho cpu_monitor (thay cho TIM2): đọc đồng hồ của Linux */
uint32_t cpu_monitor_timebase_get(void) { return sim_us() / 50U; }
