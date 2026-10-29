/**
 * @file    task_display.c
 * @brief   [APPLICATION] DisplayTask: task DUY NHẤT được dùng ma trận LED (SPI1) và OLED (I2C1)
 *          -> đóng vai trò gatekeeper cho hai thiết bị hiển thị, không cần mutex cho bus.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Chạy tuần hoàn 40 ms bằng xTaskDelayUntil (chu kỳ ổn định, không trôi như vTaskDelay).
 * Đọc dữ liệu bằng xQueuePeek từ hai mailbox qLampView và qCpuStat.
 */
#include "app_ipc.h"
#include "task_display.h"
#include "app_cfg.h"
#include "lamp_fsm.h"
#include "menu.h"
#include "effects.h"
#include "max7219.h"
#include "ssd1306.h"
#include "xprintf.h"
#include "task.h"
#include <string.h>

typedef struct {
    uint8_t rows[8U * MAX7219_NUM_DEVICES];
    uint8_t intensity;
    bool    shutdown;
} matrix_state_t;

static void update_matrix(const lamp_view_t *v, uint32_t frame, matrix_state_t *cur)
{
    if (v->level == 0U) {
        if (!cur->shutdown) {
            max7219_set_shutdown(true);
            cur->shutdown = true;
        }
        return;
    }

    matrix_state_t next;
    for (uint8_t d = 0; d < MAX7219_NUM_DEVICES; d++) {
        effects_render(v->settings.mode, frame, (uint8_t)(d * 8U), &next.rows[d * 8U]);
    }
    next.intensity = effects_intensity(v->settings.mode, frame, v->level);

    /* Chỉ gửi qua SPI khi có thay đổi: giảm thời gian chiếm bus và tải CPU */
    if (memcmp(next.rows, cur->rows, sizeof(next.rows)) != 0) {
        max7219_write_frame(next.rows);
        memcpy(cur->rows, next.rows, sizeof(cur->rows));
    }
    if (next.intensity != cur->intensity) {
        max7219_set_intensity(next.intensity);
        cur->intensity = next.intensity;
    }
    if (cur->shutdown) {
        max7219_set_shutdown(false);
        cur->shutdown = false;
    }
}

static void draw_status_page(const lamp_view_t *v, const cpu_summary_t *c)
{
    char line[24];

    ssd1306_draw_string(0, 0, " SMART LAMP FreeRTOS ", true);

    xsnprintf(line, sizeof(line), "Trang thai: %s", lamp_state_name((lamp_state_t)v->state));
    ssd1306_draw_string(0, 1, line, false);

    xsnprintf(line, sizeof(line), "Anh sang  : %s", v->dark ? "TOI" : "SANG");
    ssd1306_draw_string(0, 2, line, false);

    xsnprintf(line, sizeof(line), "Che do: %s %s", lamp_mode_name(v->settings.mode),
              v->settings.auto_on ? "[AUTO]" : "[TAY]");
    ssd1306_draw_string(0, 3, line, false);

    /* Thanh độ sáng: khung 64 px, phần tô tỉ lệ level / 16 */
    xsnprintf(line, sizeof(line), "Muc %2u/%2u", (unsigned)v->level, (unsigned)v->settings.max_level);
    ssd1306_draw_string(0, 4, line, false);
    ssd1306_draw_rect(62, 33, 64, 6);
    ssd1306_fill_rect(64, 35, (uint8_t)(v->level * 60U / LAMP_LEVEL_MAX), 2, true);

    if (v->settings.off_timer_min == 0U) {
        ssd1306_draw_string(0, 5, "Hen tat: KHONG", false);
    } else {
        xsnprintf(line, sizeof(line), "Hen tat: %u phut", (unsigned)v->settings.off_timer_min);
        ssd1306_draw_string(0, 5, line, false);
    }

    xsnprintf(line, sizeof(line), "CPU %u.%u%% (rt %u.%u%%)",
              (unsigned)(c->load_idlehook_x10 / 10U), (unsigned)(c->load_idlehook_x10 % 10U),
              (unsigned)(c->load_runtime_x10 / 10U), (unsigned)(c->load_runtime_x10 % 10U));
    ssd1306_draw_string(0, 6, line, false);

    xsnprintf(line, sizeof(line), "Ngu:%s  Up:%lus", c->sleep_enabled ? "BAT" : "TAT",
              (unsigned long)c->uptime_s);
    ssd1306_draw_string(0, 7, line, false);
}

static void draw_menu_page(const lamp_view_t *v)
{
    char value[12];
    char line[24];
    const menu_t *m = &v->menu;

    ssd1306_draw_string(0, 0, "       CAI DAT       ", true);
    for (uint8_t i = 0; i < MENU_ITEM_COUNT; i++) {
        bool selected = (i == m->item);
        bool editing = selected && (m->state == MENU_EDIT);
        /* Đang sửa thì hiện giá trị tạm (m->edit), còn lại hiện giá trị đang dùng */
        menu_item_value(i, editing ? &m->edit : &v->settings, value, sizeof(value));
        if (i == MENU_ITEM_EXIT) {
            xsnprintf(line, sizeof(line), "%c %s", selected ? '>' : ' ', menu_item_name(i));
        } else if (editing) {
            xsnprintf(line, sizeof(line), "> %-8s[%s]", menu_item_name(i), value);
        } else {
            xsnprintf(line, sizeof(line), "%c %-8s %s", selected ? '>' : ' ', menu_item_name(i), value);
        }
        ssd1306_draw_string(0, (uint8_t)(1U + i), line, selected && !editing);
    }
    ssd1306_draw_string(0, 7, (m->state == MENU_EDIT) ? "NEXT:doi   OK:luu" : "NEXT:chon  OK:vao",
                        false);
}

static void display_task(void *arg)
{
    (void)arg;
    app_wait_system_ready();

    max7219_init();
    bool oled_ok = ssd1306_init();
    log_printf("[DISP] MAX7219 x%u san sang", (unsigned)MAX7219_NUM_DEVICES);
    if (oled_ok) {
        log_printf("[DISP] OLED OK, dia chi 0x%X", (unsigned)ssd1306_address());
    } else {
        int scl, sda;
        ssd1306_bus_levels(&scl, &sda);
        log_printf("[DISP] OLED loi: %s", ssd1306_error_text());
        log_printf("[DISP] Muc bus luc ranh: SCL(PB6)=%d SDA(PB7)=%d (dung: 1 1)", scl, sda);
    }

    matrix_state_t cur;
    memset(&cur, 0, sizeof(cur));
    cur.shutdown = true;
    cur.intensity = 0xFF;   /* giá trị không hợp lệ để lần đầu chắc chắn được ghi */

    lamp_view_t view;
    memset(&view, 0, sizeof(view));
    lamp_settings_default(&view.settings);
    cpu_summary_t cpu;
    memset(&cpu, 0, sizeof(cpu));

    uint32_t frame = 0;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        (void)xQueuePeek(g_qLampView, &view, 0);   /* không có dữ liệu mới thì giữ bản cũ */
        update_matrix(&view, frame, &cur);

        if ((frame % DISPLAY_OLED_DIVIDER) == 0U) {
            (void)xQueuePeek(g_qCpuStat, &cpu, 0);
            ssd1306_clear();
            if (view.menu.state != MENU_CLOSED) {
                draw_menu_page(&view);
            } else {
                draw_status_page(&view, &cpu);
            }
            (void)ssd1306_update();               /* tự bỏ qua nếu khung không đổi */
        }

        frame++;
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}

void task_display_create(void)
{
    BaseType_t ok = xTaskCreate(display_task, "Display", STACK_DISPLAY, NULL, PRIO_DISPLAY, NULL);
    configASSERT(ok == pdPASS);
}
