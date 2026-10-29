/**
 * @file    effects.h
 * @brief   [APPLICATION] Tạo khung hình và độ sáng cho ma trận LED theo chế độ (thuần logic).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include "lamp_types.h"

/** Vẽ 8 hàng của một module. col_offset dùng khi nối nhiều module (module thứ d: d * 8). */
void    effects_render(uint8_t mode, uint32_t frame, uint8_t col_offset, uint8_t rows[8]);
/** Độ sáng MAX7219 (0..15) cho level 1..16 tại khung hình `frame`. */
uint8_t effects_intensity(uint8_t mode, uint32_t frame, uint8_t level);

#endif /* EFFECTS_H */
