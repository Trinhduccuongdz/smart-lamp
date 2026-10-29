/**
 * @file    task_console.c
 * @brief   [APPLICATION] ConsoleTask: nhận lệnh từ máy tính (USB-TTL), phân tích, chuyển thành
 *          sự kiện cho LampTask hoặc trả về thông tin trạng thái / tải CPU.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * ISR USART1 gom ký tự vào buffer; đủ một dòng thì vTaskNotifyGiveFromISR đánh thức task này.
 * Lệnh (kết thúc bằng Enter, cấu hình terminal 115200 8N1):
 *   help                 danh sách lệnh
 *   status               trạng thái đèn
 *   on | off | toggle    bật / tắt / đảo đèn
 *   mode <0..2>          0 FULL, 1 BREATH, 2 WAVE
 *   bright <1..16>       độ sáng tối đa
 *   timer <phut>         hẹn tắt sau N phút khi đèn sáng hẳn (0 = không)
 *   auto <0|1>           tự động theo cảm biến
 *   cpu                  bảng tải CPU + stack của từng task
 *   sleep <0|1>          cho CPU ngủ (__WFI) khi rảnh
 *   log <0|1>            in tải CPU mỗi giây (để vẽ đồ thị trên máy tính)
 */
#include "app_ipc.h"
#include "task_console.h"
#include "task_monitor.h"
#include "app_cfg.h"
#include "console.h"
#include "cpu_monitor.h"
#include "lamp_fsm.h"
#include "task.h"
#include <string.h>

static TaskHandle_t s_task;

/* Ngữ cảnh ISR: console middleware báo đã có một dòng hoàn chỉnh */
static void on_line_isr(void)
{
    BaseType_t woken = pdFALSE;
    if (s_task != NULL) {
        vTaskNotifyGiveFromISR(s_task, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

/* Tách "lenh so": trả về con trỏ tới tham số (hoặc NULL) và cắt chuỗi lệnh */
static const char *split_arg(char *line)
{
    char *p = line;
    while (*p != '\0' && *p != ' ') { p++; }
    if (*p == '\0') { return NULL; }
    *p++ = '\0';
    while (*p == ' ') { p++; }
    return (*p != '\0') ? p : NULL;
}

static bool parse_uint(const char *s, uint32_t *out)
{
    if (s == NULL || *s == '\0') { return false; }
    uint32_t v = 0;
    while (*s != '\0') {
        if (*s < '0' || *s > '9') { return false; }
        v = v * 10U + (uint32_t)(*s++ - '0');
        if (v > 100000U) { return false; }
    }
    *out = v;
    return true;
}

static void cmd_help(void)
{
    log_printf("Lenh: help status on off toggle");
    log_printf("      mode <0-2> bright <1-16> timer <phut> auto <0|1>");
    log_printf("      cpu  sleep <0|1>  log <0|1>");
}

static void cmd_status(void)
{
    lamp_view_t v;
    if (xQueuePeek(g_qLampView, &v, 0) != pdPASS) {
        log_printf("Chua co du lieu");
        return;
    }
    log_printf("Den: %s  muc %u/%u  troi %s  (toi %u lan)",
               lamp_state_name((lamp_state_t)v.state), (unsigned)v.level,
               (unsigned)v.settings.max_level, v.dark ? "TOI" : "SANG", (unsigned)v.dark_events);
    log_printf("Che do %s  auto %u  hen tat %u ph", lamp_mode_name(v.settings.mode),
               (unsigned)v.settings.auto_on, (unsigned)v.settings.off_timer_min);
}

static void cmd_cpu(void)
{
    static cpu_stats_t st;   /* static: tránh chiếm ~240 byte stack của task */
    if (!cpu_monitor_get(&st, pdMS_TO_TICKS(50))) {
        log_printf("Khong lay duoc thong ke");
        return;
    }
    log_printf("Uptime %lus  sleep %s  heap trong %lu (min %lu) B",
               (unsigned long)st.uptime_s, st.sleep_enabled ? "BAT" : "TAT",
               (unsigned long)st.heap_free, (unsigned long)st.heap_min_free);
    log_printf("Tai CPU: idle-hook %u.%u%%  run-time %u.%u%%",
               (unsigned)(st.load_idlehook_x10 / 10U), (unsigned)(st.load_idlehook_x10 % 10U),
               (unsigned)(st.load_runtime_x10 / 10U), (unsigned)(st.load_runtime_x10 % 10U));
    log_printf("  (idle %lu lan/s, hieu chuan %lu lan/s)",
               (unsigned long)st.idle_count, (unsigned long)st.idle_max);
    log_printf("%-10s %4s %7s %10s", "Task", "Prio", "CPU", "Stack trong");
    for (uint8_t i = 0; i < st.task_count; i++) {
        const cpu_task_stat_t *t = &st.tasks[i];
        log_printf("%-10s %4u %4u.%u%% %6u word", t->name, (unsigned)t->priority,
                   (unsigned)(t->cpu_x10 / 10U), (unsigned)(t->cpu_x10 % 10U),
                   (unsigned)t->stack_free_words);
    }
}

static void handle_line(char *line)
{
    const char *arg = split_arg(line);
    uint32_t v = 0;
    bool has_num = parse_uint(arg, &v);

    if (strcmp(line, "help") == 0)        { cmd_help(); }
    else if (strcmp(line, "status") == 0) { cmd_status(); }
    else if (strcmp(line, "on") == 0)     { (void)app_post_event(EV_CMD_LAMP, 0, 1, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "off") == 0)    { (void)app_post_event(EV_CMD_LAMP, 0, 0, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "toggle") == 0) { (void)app_post_event(EV_CMD_LAMP, 0, 2, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "mode") == 0 && has_num && v < LAMP_MODE_COUNT) {
        (void)app_post_event(EV_CMD_SET, SET_MODE, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "bright") == 0 && has_num && v >= 1U && v <= LAMP_LEVEL_MAX) {
        (void)app_post_event(EV_CMD_SET, SET_MAX_LEVEL, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "timer") == 0 && has_num && v <= LAMP_OFF_TIMER_MAX) {
        (void)app_post_event(EV_CMD_SET, SET_OFF_TIMER, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "auto") == 0 && has_num && v <= 1U) {
        (void)app_post_event(EV_CMD_SET, SET_AUTO, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "cpu") == 0) {
        cmd_cpu();
    } else if (strcmp(line, "sleep") == 0 && has_num && v <= 1U) {
        cpu_monitor_set_sleep(v == 1U);
        log_printf("CPU ngu khi ranh: %s", (v == 1U) ? "BAT" : "TAT");
    } else if (strcmp(line, "log") == 0 && has_num && v <= 1U) {
        monitor_set_periodic_log(v == 1U);
    } else {
        log_printf("Lenh hoac tham so khong hop le (%s). Go 'help'", line);
        return;
    }
}

static void console_task(void *arg)
{
    (void)arg;
    char line[CONSOLE_LINE_MAX];

    app_wait_system_ready();
    log_printf("=== Smart Lamp (FreeRTOS %s) - go 'help' ===", tskKERNEL_VERSION_NUMBER);

    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   /* ngủ đến khi ISR báo có dòng mới */
        while (console_readline(line, sizeof(line))) {
            handle_line(line);
        }
    }
}

void task_console_create(void)
{
    console_init(CONSOLE_BAUD, on_line_isr);
    BaseType_t ok = xTaskCreate(console_task, "Console", STACK_CONSOLE, NULL, PRIO_CONSOLE, &s_task);
    configASSERT(ok == pdPASS);
}
