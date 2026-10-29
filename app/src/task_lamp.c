/**
 * @file    task_lamp.c
 * @brief   [APPLICATION] LampTask: nhận sự kiện từ qEvents, chạy máy trạng thái đèn + menu,
 *          điều khiển software timer sáng/tối dần và hẹn tắt, công bố trạng thái qua qLampView.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Firmware hướng sự kiện (event-driven): task chỉ chạy khi có sự kiện trong qEvents,
 * còn lại nằm ở trạng thái Blocked, không tốn CPU.
 */
#include "app_ipc.h"
#include "task_lamp.h"
#include "app_cfg.h"
#include "lamp_fsm.h"
#include "menu.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t s_tmr_fade;   /* auto-reload 100 ms: mỗi nhịp tăng/giảm level 1 bậc */
static TimerHandle_t s_tmr_off;    /* one-shot: hẹn giờ tắt */

static lamp_fsm_t      s_fsm;
static menu_t          s_menu;
static lamp_settings_t s_settings;
static uint16_t        s_dark_events;

/* Callback software timer chạy trong Timer daemon: chỉ gửi sự kiện, không chờ (block time 0) */
static void on_fade_timer(TimerHandle_t t)
{
    (void)t;
    (void)app_post_event(EV_FADE_TICK, 0, 0, 0);
}

static void on_off_timer(TimerHandle_t t)
{
    (void)t;
    (void)app_post_event(EV_OFF_TIMEOUT, 0, 0, 0);
}

static void apply_actions(uint8_t act)
{
    const TickType_t wait = pdMS_TO_TICKS(10);
    if ((act & LAMP_ACT_FADE_STOP) != 0U)      { (void)xTimerStop(s_tmr_fade, wait); }
    if ((act & LAMP_ACT_OFF_TIMER_STOP) != 0U) { (void)xTimerStop(s_tmr_off, wait); }
    if ((act & LAMP_ACT_FADE_START) != 0U)     { (void)xTimerReset(s_tmr_fade, wait); }
    if ((act & LAMP_ACT_OFF_TIMER_START) != 0U) {
        TickType_t period = pdMS_TO_TICKS((uint32_t)s_settings.off_timer_min * 60000UL);
        (void)xTimerChangePeriod(s_tmr_off, period, wait);   /* đổi chu kỳ đồng thời khởi động */
    }
}

static void publish_view(void)
{
    lamp_view_t v;
    v.state = (uint8_t)s_fsm.state;
    v.level = s_fsm.level;
    v.dark = s_fsm.dark;
    v.settings = s_settings;
    v.menu = s_menu;
    v.dark_events = s_dark_events;
    (void)xQueueOverwrite(g_qLampView, &v);   /* mailbox: luôn giữ giá trị mới nhất */
}

static void apply_setting(uint8_t param, uint16_t value)
{
    switch (param) {
    case SET_MODE:      s_settings.mode = (uint8_t)value; break;
    case SET_MAX_LEVEL: s_settings.max_level = (uint8_t)value; break;
    case SET_OFF_TIMER: s_settings.off_timer_min = (uint8_t)(value > 255U ? 255U : value); break;
    case SET_AUTO:      s_settings.auto_on = (uint8_t)value; break;
    default: break;
    }
    lamp_settings_sanitize(&s_settings);
}

static uint8_t handle_event(const app_event_t *ev)
{
    switch (ev->type) {
    case EV_SENSOR_DARK:
        s_dark_events++;
        return lamp_fsm_handle(&s_fsm, LAMP_IN_DARK, &s_settings);
    case EV_SENSOR_LIGHT:
        return lamp_fsm_handle(&s_fsm, LAMP_IN_LIGHT, &s_settings);
    case EV_FADE_TICK:
        return lamp_fsm_handle(&s_fsm, LAMP_IN_FADE_TICK, &s_settings);
    case EV_OFF_TIMEOUT:
        log_printf("[LAMP] Het gio hen tat");
        return lamp_fsm_handle(&s_fsm, LAMP_IN_OFF_TIMEOUT, &s_settings);

    case EV_BTN_NEXT: case EV_BTN_OK: case EV_BTN_NEXT_HOLD: case EV_BTN_OK_HOLD: {
        static const menu_key_t key_map[] = { MENU_KEY_NEXT, MENU_KEY_OK, MENU_KEY_NEXT_HOLD, MENU_KEY_OK_HOLD };
        uint8_t res = menu_handle(&s_menu, key_map[ev->type - EV_BTN_NEXT], &s_settings);
        if ((res & MENU_RES_TOGGLE_LAMP) != 0U) {
            return lamp_fsm_handle(&s_fsm, LAMP_IN_TOGGLE, &s_settings);
        }
        if ((res & MENU_RES_SETTINGS) != 0U) {
            log_printf("[LAMP] Cai dat: mode=%s max=%u hen=%u auto=%u",
                       lamp_mode_name(s_settings.mode), (unsigned)s_settings.max_level,
                       (unsigned)s_settings.off_timer_min, (unsigned)s_settings.auto_on);
            return lamp_fsm_handle(&s_fsm, LAMP_IN_SETTINGS, &s_settings);
        }
        return LAMP_ACT_NONE;
    }

    case EV_CMD_LAMP:
        if (ev->value == 0U) { return lamp_fsm_handle(&s_fsm, LAMP_IN_OFF_REQ, &s_settings); }
        if (ev->value == 1U) { return lamp_fsm_handle(&s_fsm, LAMP_IN_ON_REQ, &s_settings); }
        return lamp_fsm_handle(&s_fsm, LAMP_IN_TOGGLE, &s_settings);

    case EV_CMD_SET:
        apply_setting(ev->param, ev->value);
        return lamp_fsm_handle(&s_fsm, LAMP_IN_SETTINGS, &s_settings);

    default:
        return LAMP_ACT_NONE;
    }
}

static void lamp_task(void *arg)
{
    (void)arg;
    app_wait_system_ready();
    publish_view();

    for (;;) {
        app_event_t ev;
        if (xQueueReceive(g_qEvents, &ev, portMAX_DELAY) != pdPASS) {
            continue;
        }
        lamp_state_t before = s_fsm.state;
        uint8_t act = handle_event(&ev);
        apply_actions(act);
        if (s_fsm.state != before) {
            log_printf("[LAMP] %s -> %s", lamp_state_name(before), lamp_state_name(s_fsm.state));
        }
        publish_view();
    }
}

void task_lamp_create(void)
{
    lamp_settings_default(&s_settings);
    lamp_fsm_init(&s_fsm);
    menu_init(&s_menu);

    s_tmr_fade = xTimerCreate("fade", pdMS_TO_TICKS(FADE_STEP_MS), pdTRUE, NULL, on_fade_timer);
    s_tmr_off  = xTimerCreate("off", pdMS_TO_TICKS(60000U), pdFALSE, NULL, on_off_timer);
    configASSERT(s_tmr_fade && s_tmr_off);

    BaseType_t ok = xTaskCreate(lamp_task, "Lamp", STACK_LAMP, NULL, PRIO_LAMP, NULL);
    configASSERT(ok == pdPASS);
}
