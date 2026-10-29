/**
 * @file    menu.h
 * @brief   [APPLICATION] Menu cài đặt trên OLED điều khiển bằng 2 nút (máy trạng thái thuần logic).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 *  Trạng thái   NEXT (nhấn ngắn)       OK (nhấn ngắn)                 Nhấn giữ (>= 0.8 s)
 *  ----------   --------------------   ----------------------------   -------------------
 *  CLOSED       mở menu (NAV)          bật/tắt đèn                    -
 *  NAV          sang mục kế tiếp       vào sửa mục (EDIT) / Thoát     đóng menu
 *  EDIT         đổi giá trị            lưu, quay lại NAV              hủy, quay lại NAV
 */
#ifndef MENU_H
#define MENU_H

#include <stddef.h>
#include "lamp_types.h"

typedef enum { MENU_KEY_NEXT = 0, MENU_KEY_OK, MENU_KEY_NEXT_HOLD, MENU_KEY_OK_HOLD } menu_key_t;
typedef enum { MENU_CLOSED = 0, MENU_NAV, MENU_EDIT } menu_state_t;
typedef enum {
    MENU_ITEM_MODE = 0, MENU_ITEM_BRIGHT, MENU_ITEM_TIMER, MENU_ITEM_AUTO, MENU_ITEM_EXIT,
    MENU_ITEM_COUNT
} menu_item_t;

typedef struct {
    uint8_t         state;   /* menu_state_t */
    uint8_t         item;    /* menu_item_t đang chọn */
    lamp_settings_t edit;    /* bản sao đang sửa (chỉ ghi vào cài đặt thật khi nhấn OK) */
} menu_t;

/* Kết quả trả về (bitmask) */
#define MENU_RES_NONE        0x00U
#define MENU_RES_TOGGLE_LAMP 0x01U
#define MENU_RES_SETTINGS    0x02U   /* *settings vừa được ghi giá trị mới */
#define MENU_RES_REDRAW      0x04U

void        menu_init(menu_t *m);
uint8_t     menu_handle(menu_t *m, menu_key_t key, lamp_settings_t *settings);
const char *menu_item_name(uint8_t item);
/** Ghi chuỗi giá trị của `item` lấy từ `s` vào out (ví dụ "BREATH", "12", "5 ph", "BAT"). */
void        menu_item_value(uint8_t item, const lamp_settings_t *s, char *out, size_t size);

#endif /* MENU_H */
