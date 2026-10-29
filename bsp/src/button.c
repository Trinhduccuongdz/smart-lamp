/**
 * @file    button.c
 * @brief   [BSP] Nút nhấn dựa trên driver gpio + exti.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#include "button.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_exti.h"
#include <stddef.h>

static button_edge_cb_t s_cb;

static void on_exti(uint8_t line)
{
    if (s_cb == NULL) { return; }
    if (line == BOARD_BTN_NEXT_PIN)     { s_cb(BUTTON_NEXT); }
    else if (line == BOARD_BTN_OK_PIN)  { s_cb(BUTTON_OK); }
}

void button_init(button_edge_cb_t cb)
{
    s_cb = cb;
    gpio_config_input_pull(BOARD_BTN_NEXT_PORT, BOARD_BTN_NEXT_PIN, GPIO_PULL_UP);
    gpio_config_input_pull(BOARD_BTN_OK_PORT, BOARD_BTN_OK_PIN, GPIO_PULL_UP);
    exti_config(BOARD_BTN_NEXT_PORT, BOARD_BTN_NEXT_PIN, EXTI_EDGE_BOTH, BOARD_IRQ_PRIO_EXTI, on_exti);
    exti_config(BOARD_BTN_OK_PORT, BOARD_BTN_OK_PIN, EXTI_EDGE_BOTH, BOARD_IRQ_PRIO_EXTI, on_exti);
}

bool button_is_pressed(button_id_t id)
{
    if (id == BUTTON_NEXT) { return !gpio_read(BOARD_BTN_NEXT_PORT, BOARD_BTN_NEXT_PIN); }
    return !gpio_read(BOARD_BTN_OK_PORT, BOARD_BTN_OK_PIN);
}
