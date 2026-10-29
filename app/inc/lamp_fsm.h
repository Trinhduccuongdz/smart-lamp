/**
 * @file    lamp_fsm.h
 * @brief   [APPLICATION] Máy trạng thái của đèn (xem docs/4_lamp_fsm.png).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Module THUẦN LOGIC: không gọi FreeRTOS, không đụng phần cứng. Đầu vào là sự kiện,
 * đầu ra là trạng thái mới + các "hành động" (bật/tắt timer) để LampTask thực hiện.
 * Nhờ vậy có thể unit test trên máy tính (thư mục test/).
 */
#ifndef LAMP_FSM_H
#define LAMP_FSM_H

#include "lamp_types.h"

typedef enum {
    LAMP_IN_DARK = 0,     /* cảm biến: trời tối (đã chống nhiễu) */
    LAMP_IN_LIGHT,        /* cảm biến: trời sáng */
    LAMP_IN_ON_REQ,       /* yêu cầu bật (lệnh "on") */
    LAMP_IN_OFF_REQ,      /* yêu cầu tắt (lệnh "off") */
    LAMP_IN_TOGGLE,       /* đảo trạng thái (nút OK, lệnh "toggle") */
    LAMP_IN_FADE_TICK,    /* nhịp sáng/tối dần (tmrFade, 100 ms) */
    LAMP_IN_OFF_TIMEOUT,  /* hết giờ hẹn tắt (tmrOff) */
    LAMP_IN_SETTINGS      /* cài đặt vừa thay đổi */
} lamp_input_t;

/* Hành động đầu ra (bitmask). LampTask thực hiện các lệnh STOP trước, START sau. */
#define LAMP_ACT_NONE            0x00U
#define LAMP_ACT_FADE_START      0x01U
#define LAMP_ACT_FADE_STOP       0x02U
#define LAMP_ACT_OFF_TIMER_START 0x04U
#define LAMP_ACT_OFF_TIMER_STOP  0x08U

typedef struct {
    lamp_state_t state;
    uint8_t      level;    /* 0..max_level */
    bool         dark;     /* trạng thái cảm biến gần nhất */
} lamp_fsm_t;

void        lamp_fsm_init(lamp_fsm_t *f);
uint8_t     lamp_fsm_handle(lamp_fsm_t *f, lamp_input_t in, const lamp_settings_t *s);
const char *lamp_state_name(lamp_state_t st);
const char *lamp_mode_name(uint8_t mode);

void lamp_settings_default(lamp_settings_t *s);
/** Kẹp các giá trị về miền hợp lệ. */
void lamp_settings_sanitize(lamp_settings_t *s);

#endif /* LAMP_FSM_H */
