/**
 * @file    lamp_types.h
 * @brief   [APPLICATION] Kiểu dữ liệu thuần (không phụ thuộc phần cứng/FreeRTOS) của đèn.
 * @owner   Người 1 (Đầu vào + điều khiển đèn) - dùng chung cả nhóm
 */
#ifndef LAMP_TYPES_H
#define LAMP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define LAMP_LEVEL_MAX       16U   /* level 0 = tắt, 1..16 -> intensity MAX7219 0..15 */
#define LAMP_OFF_TIMER_MAX   240U  /* phút */

typedef enum {
    LAMP_MODE_FULL = 0,   /* sáng đều toàn bộ ma trận */
    LAMP_MODE_BREATH,     /* "thở": độ sáng lên xuống chậm */
    LAMP_MODE_WAVE,       /* sóng chạy */
    LAMP_MODE_COUNT
} lamp_mode_t;

typedef enum {
    LAMP_OFF = 0,
    LAMP_FADE_IN,
    LAMP_ON,
    LAMP_FADE_OUT
} lamp_state_t;

typedef struct {
    uint8_t mode;           /* lamp_mode_t */
    uint8_t max_level;      /* 1..16 */
    uint8_t off_timer_min;  /* 0 = không hẹn giờ */
    uint8_t auto_on;        /* 1 = tự bật/tắt theo cảm biến */
} lamp_settings_t;

#endif /* LAMP_TYPES_H */
