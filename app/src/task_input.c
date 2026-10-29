/**
 * @file    task_input.c
 * @brief   [APPLICATION] InputTask: đọc cảm biến quang + 2 nút, chống nhiễu, sinh sự kiện vào qEvents.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Luồng xử lý (kỹ thuật "trì hoãn xử lý ngắt" trong slide FreeRTOS phần 2):
 *   1. ISR EXTI chỉ làm một việc: gửi task notification (bit cạnh) cho InputTask.
 *   2. InputTask khởi động lại (reset) software timer chống nhiễu của nguồn đó.
 *   3. Khi tín hiệu đứng yên đủ lâu, timer hết hạn -> callback gửi notification (bit ổn định).
 *   4. InputTask đọc mức chân, so với trạng thái cũ -> sinh sự kiện DARK/LIGHT, BTN_xxx.
 */
#include "app_ipc.h"
#include "task_input.h"
#include "app_cfg.h"
#include "light_sensor.h"
#include "button.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>

/* Bit notification. Nguồn: 0 = cảm biến, 1 = NEXT, 2 = OK */
#define SRC_SENSOR  0U
#define SRC_NEXT    1U
#define SRC_OK      2U
#define SRC_COUNT   3U
#define NB_EDGE(src)    (1UL << (src))          /* từ ISR: chân vừa đổi mức */
#define NB_STABLE(src)  (1UL << (8U + (src)))   /* từ timer: chân đã ổn định */

static TaskHandle_t  s_task;
static TimerHandle_t s_debounce[SRC_COUNT];

/* ---------- Ngữ cảnh ISR (callback từ middleware) ---------- */
static void notify_from_isr(uint32_t bits)
{
    BaseType_t woken = pdFALSE;
    if (s_task != NULL) {
        xTaskNotifyFromISR(s_task, bits, eSetBits, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

static void on_sensor_edge_isr(void)
{
    notify_from_isr(NB_EDGE(SRC_SENSOR));
}

static void on_button_edge_isr(button_id_t id)
{
    notify_from_isr(NB_EDGE(id == BUTTON_NEXT ? SRC_NEXT : SRC_OK));
}

/* ---------- Ngữ cảnh Timer daemon ---------- */
static void on_debounce_expired(TimerHandle_t t)
{
    uint32_t src = (uint32_t)(uintptr_t)pvTimerGetTimerID(t);
    xTaskNotify(s_task, NB_STABLE(src), eSetBits);
}

/* ---------- Ngữ cảnh InputTask ---------- */
static void handle_sensor(bool *dark_state, TickType_t *dark_since, uint16_t *dark_count)
{
    bool dark = light_sensor_is_dark();
    if (dark == *dark_state) {
        return;                                  /* nhiễu: quay về mức cũ */
    }
    *dark_state = dark;
    TickType_t now = xTaskGetTickCount();
    if (dark) {
        (*dark_count)++;
        *dark_since = now;
        log_printf("[IN] Troi TOI (lan %u)", (unsigned)*dark_count);
        (void)app_post_event(EV_SENSOR_DARK, 0, 0, pdMS_TO_TICKS(20));
    } else {
        uint32_t secs = (uint32_t)((now - *dark_since) / configTICK_RATE_HZ);
        log_printf("[IN] Troi SANG (toi trong %lu s)", (unsigned long)secs);
        (void)app_post_event(EV_SENSOR_LIGHT, 0, 0, pdMS_TO_TICKS(20));
    }
}

static void handle_button(uint32_t src, bool *pressed_state, TickType_t *press_tick)
{
    button_id_t id = (src == SRC_NEXT) ? BUTTON_NEXT : BUTTON_OK;
    bool pressed = button_is_pressed(id);
    if (pressed == *pressed_state) {
        return;
    }
    *pressed_state = pressed;
    if (pressed) {
        *press_tick = xTaskGetTickCount();       /* ghi lại lúc nhấn */
        return;
    }
    /* Nhả nút: phân loại nhấn ngắn / nhấn giữ theo thời gian giữ */
    TickType_t held = xTaskGetTickCount() - *press_tick;
    bool hold = held >= pdMS_TO_TICKS(BUTTON_HOLD_MS);
    uint8_t type;
    if (id == BUTTON_NEXT) { type = hold ? EV_BTN_NEXT_HOLD : EV_BTN_NEXT; }
    else                   { type = hold ? EV_BTN_OK_HOLD : EV_BTN_OK; }
    (void)app_post_event(type, 0, 0, pdMS_TO_TICKS(20));
}

static void input_task(void *arg)
{
    (void)arg;
    bool       dark = false;
    TickType_t dark_since = 0;
    uint16_t   dark_count = 0;
    bool       pressed[SRC_COUNT] = { false, false, false };
    TickType_t press_tick[SRC_COUNT] = { 0, 0, 0 };

    app_wait_system_ready();

    light_sensor_init(on_sensor_edge_isr);
    button_init(on_button_edge_isr);

    /* Đồng bộ trạng thái ban đầu của cảm biến cho LampTask */
    dark = light_sensor_is_dark();
    if (dark) { dark_count = 1; dark_since = xTaskGetTickCount(); }
    log_printf("[IN] Khoi dong: troi %s", dark ? "TOI" : "SANG");
    (void)app_post_event(dark ? EV_SENSOR_DARK : EV_SENSOR_LIGHT, 0, 0, portMAX_DELAY);

    for (;;) {
        uint32_t bits = 0;
        (void)xTaskNotifyWait(0, 0xFFFFFFFFUL, &bits, portMAX_DELAY);

        for (uint32_t src = 0; src < SRC_COUNT; src++) {
            if ((bits & NB_EDGE(src)) != 0U) {
                /* Có cạnh mới: đếm lại thời gian chống nhiễu từ đầu */
                (void)xTimerReset(s_debounce[src], pdMS_TO_TICKS(10));
            }
            if ((bits & NB_STABLE(src)) != 0U) {
                if (src == SRC_SENSOR) {
                    handle_sensor(&dark, &dark_since, &dark_count);
                } else {
                    handle_button(src, &pressed[src], &press_tick[src]);
                }
            }
        }
    }
}

void task_input_create(void)
{
    s_debounce[SRC_SENSOR] = xTimerCreate("dbSens", pdMS_TO_TICKS(DEBOUNCE_SENSOR_MS), pdFALSE,
                                          (void *)(uintptr_t)SRC_SENSOR, on_debounce_expired);
    s_debounce[SRC_NEXT]   = xTimerCreate("dbNext", pdMS_TO_TICKS(DEBOUNCE_BUTTON_MS), pdFALSE,
                                          (void *)(uintptr_t)SRC_NEXT, on_debounce_expired);
    s_debounce[SRC_OK]     = xTimerCreate("dbOk", pdMS_TO_TICKS(DEBOUNCE_BUTTON_MS), pdFALSE,
                                          (void *)(uintptr_t)SRC_OK, on_debounce_expired);
    configASSERT(s_debounce[0] && s_debounce[1] && s_debounce[2]);

    BaseType_t ok = xTaskCreate(input_task, "Input", STACK_INPUT, NULL, PRIO_INPUT, &s_task);
    configASSERT(ok == pdPASS);
}
