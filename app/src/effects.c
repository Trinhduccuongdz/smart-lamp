/**
 * @file    effects.c
 * @brief   [APPLICATION] Hiệu ứng FULL / BREATH / WAVE cho ma trận 8x8.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * DisplayTask gọi mỗi 40 ms (25 khung hình/giây).
 */
#include "effects.h"

#define BREATH_PERIOD  64U   /* 64 khung x 40 ms = 2.56 s cho một chu kỳ thở */
#define WAVE_SPEED     3U    /* sóng dịch 1 cột sau mỗi 3 khung */

/* Chiều cao cột (0..8) theo dạng sóng sin rời rạc */
static const uint8_t k_wave[16] = { 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 1, 1, 2, 3 };

void effects_render(uint8_t mode, uint32_t frame, uint8_t col_offset, uint8_t rows[8])
{
    if (mode != LAMP_MODE_WAVE) {
        for (uint8_t r = 0; r < 8U; r++) { rows[r] = 0xFFU; }   /* sáng toàn bộ */
        return;
    }
    for (uint8_t r = 0; r < 8U; r++) { rows[r] = 0x00U; }
    for (uint8_t c = 0; c < 8U; c++) {
        uint8_t h = k_wave[(uint32_t)(c + col_offset + frame / WAVE_SPEED) % 16U];
        /* tô từ hàng dưới (r = 7) lên h hàng; bit 7 = cột trái */
        for (uint8_t k = 0; k < h; k++) {
            rows[7U - k] |= (uint8_t)(0x80U >> c);
        }
    }
}

uint8_t effects_intensity(uint8_t mode, uint32_t frame, uint8_t level)
{
    if (level == 0U) { return 0U; }
    uint8_t top = (uint8_t)(level - 1U);           /* level 1..16 -> 0..15 */
    if (mode != LAMP_MODE_BREATH) {
        return top;
    }
    uint32_t p = frame % BREATH_PERIOD;
    uint32_t tri = (p < BREATH_PERIOD / 2U) ? p : (BREATH_PERIOD - 1U - p);   /* 0..31..0 */
    return (uint8_t)((top * tri) / (BREATH_PERIOD / 2U - 1U));
}
