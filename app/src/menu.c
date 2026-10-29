/**
 * @file    menu.c
 * @brief   [APPLICATION] Cài đặt menu 2 nút.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#include "menu.h"
#include "lamp_fsm.h"

static const uint8_t k_timer_steps[] = { 0, 1, 5, 15, 30, 60 };

void menu_init(menu_t *m)
{
    m->state = MENU_CLOSED;
    m->item = MENU_ITEM_MODE;
    lamp_settings_default(&m->edit);
}

static void cycle_value(menu_t *m)
{
    lamp_settings_t *e = &m->edit;
    switch (m->item) {
    case MENU_ITEM_MODE:
        e->mode = (uint8_t)((e->mode + 1U) % LAMP_MODE_COUNT);
        break;
    case MENU_ITEM_BRIGHT:
        /* 2, 4, ..., 16 rồi quay về 2 */
        e->max_level = (uint8_t)((e->max_level / 2U + 1U) * 2U);
        if (e->max_level > LAMP_LEVEL_MAX) { e->max_level = 2U; }
        break;
    case MENU_ITEM_TIMER: {
        uint8_t next = 0;
        for (size_t i = 0; i < sizeof(k_timer_steps); i++) {
            if (k_timer_steps[i] > e->off_timer_min) { next = k_timer_steps[i]; break; }
        }
        e->off_timer_min = next;   /* vượt bước cuối -> về 0 (tắt hẹn giờ) */
        break;
    }
    case MENU_ITEM_AUTO:
        e->auto_on = (e->auto_on != 0U) ? 0U : 1U;
        break;
    default:
        break;
    }
}

uint8_t menu_handle(menu_t *m, menu_key_t key, lamp_settings_t *settings)
{
    switch (m->state) {
    case MENU_CLOSED:
        if (key == MENU_KEY_OK)   { return MENU_RES_TOGGLE_LAMP; }
        if (key == MENU_KEY_NEXT) {
            m->state = MENU_NAV;
            m->item = MENU_ITEM_MODE;
            return MENU_RES_REDRAW;
        }
        return MENU_RES_NONE;

    case MENU_NAV:
        if (key == MENU_KEY_NEXT) {
            m->item = (uint8_t)((m->item + 1U) % MENU_ITEM_COUNT);
        } else if (key == MENU_KEY_OK) {
            if (m->item == MENU_ITEM_EXIT) {
                m->state = MENU_CLOSED;
            } else {
                m->edit = *settings;
                m->state = MENU_EDIT;
            }
        } else {
            m->state = MENU_CLOSED;          /* nhấn giữ: đóng menu */
        }
        return MENU_RES_REDRAW;

    case MENU_EDIT:
        if (key == MENU_KEY_NEXT) {
            cycle_value(m);
            return MENU_RES_REDRAW;
        }
        m->state = MENU_NAV;
        if (key == MENU_KEY_OK) {
            *settings = m->edit;             /* lưu */
            lamp_settings_sanitize(settings);
            return MENU_RES_SETTINGS | MENU_RES_REDRAW;
        }
        return MENU_RES_REDRAW;              /* nhấn giữ: hủy */

    default:
        m->state = MENU_CLOSED;
        return MENU_RES_REDRAW;
    }
}

const char *menu_item_name(uint8_t item)
{
    switch (item) {
    case MENU_ITEM_MODE:   return "Che do";
    case MENU_ITEM_BRIGHT: return "Do sang";
    case MENU_ITEM_TIMER:  return "Hen tat";
    case MENU_ITEM_AUTO:   return "Tu dong";
    case MENU_ITEM_EXIT:   return "Thoat";
    default:               return "?";
    }
}

/* Ghi số nguyên dương nhỏ vào chuỗi (tránh phụ thuộc thư viện in để giữ module thuần) */
static size_t put_uint(char *out, size_t size, unsigned v)
{
    char tmp[4];
    size_t n = 0, k = 0;
    do { tmp[n++] = (char)('0' + v % 10U); v /= 10U; } while (v != 0U && n < sizeof(tmp));
    while (n > 0U && k + 1U < size) { out[k++] = tmp[--n]; }
    out[k] = '\0';
    return k;
}

static void copy_str(char *out, size_t size, const char *s)
{
    size_t k = 0;
    while (s[k] != '\0' && k + 1U < size) { out[k] = s[k]; k++; }
    out[k] = '\0';
}

void menu_item_value(uint8_t item, const lamp_settings_t *s, char *out, size_t size)
{
    if (size == 0U) { return; }
    switch (item) {
    case MENU_ITEM_MODE:
        copy_str(out, size, lamp_mode_name(s->mode));
        break;
    case MENU_ITEM_BRIGHT:
        (void)put_uint(out, size, s->max_level);
        break;
    case MENU_ITEM_TIMER:
        if (s->off_timer_min == 0U) {
            copy_str(out, size, "KHONG");
        } else {
            size_t k = put_uint(out, size, s->off_timer_min);
            copy_str(out + k, size - k, " ph");
        }
        break;
    case MENU_ITEM_AUTO:
        copy_str(out, size, (s->auto_on != 0U) ? "BAT" : "TAT");
        break;
    default:
        out[0] = '\0';
        break;
    }
}
