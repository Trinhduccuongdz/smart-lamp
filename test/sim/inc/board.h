/* [MÔ PHỎNG] bản giả của board.h */
#ifndef BOARD_H
#define BOARD_H
#define BOARD_IRQ_PRIO_TIM2 4U
#define BOARD_IRQ_PRIO_UART 6U
#define BOARD_IRQ_PRIO_EXTI 7U
void board_init(void); void board_led_toggle(void); void board_led_set(int on);
#endif
