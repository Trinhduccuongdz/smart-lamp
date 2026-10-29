/* [MÔ PHỎNG] bản giả của uart.h */
#ifndef HAL_UART_H
#define HAL_UART_H
#include <stdint.h>
#include <stdbool.h>
typedef void (*uart_rx_callback_t)(uint8_t byte);
typedef void (*uart_tx_done_callback_t)(void);
void uart1_init(uint32_t baud, uint8_t irq_prio, uart_rx_callback_t rx_cb, uart_tx_done_callback_t tx_done_cb);
bool uart1_send_dma(const uint8_t *buf, uint16_t len);
void uart1_send_blocking(const char *s);
#endif
