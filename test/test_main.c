/**
 * @file    test_main.c
 * @brief   Unit test chạy trên máy tính cho các module THUẦN LOGIC (không cần bo mạch):
 *          lamp_fsm.c, menu.c, effects.c (tầng Application) và xprintf.c (tầng Middleware).
 *          Chạy: make test
 * @owner   Người 1 (Đầu vào + điều khiển đèn) - viết test cho máy trạng thái của mình;
 *          Người 2 và 3 bổ sung test cho effects.c và xprintf.c
 */
#include <stdio.h>
#include <string.h>
#include "lamp_fsm.h"
#include "menu.h"
#include "effects.h"
#include "xprintf.h"

static int s_run, s_fail;

#define CHECK(cond) do { s_run++; if (!(cond)) { s_fail++; \
    printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* Gửi n nhịp FADE_TICK, trả về OR của các hành động */
static uint8_t ticks(lamp_fsm_t *f, const lamp_settings_t *s, int n)
{
    uint8_t act = 0;
    for (int i = 0; i < n; i++) { act |= lamp_fsm_handle(f, LAMP_IN_FADE_TICK, s); }
    return act;
}

static void test_fsm_auto_cycle(void)
{
    lamp_fsm_t f; lamp_settings_t s;
    lamp_fsm_init(&f); lamp_settings_default(&s);

    CHECK(f.state == LAMP_OFF && f.level == 0);
    uint8_t act = lamp_fsm_handle(&f, LAMP_IN_DARK, &s);
    CHECK(f.state == LAMP_FADE_IN);
    CHECK(act & LAMP_ACT_FADE_START);

    ticks(&f, &s, 15);
    CHECK(f.state == LAMP_FADE_IN && f.level == 15);
    act = ticks(&f, &s, 1);
    CHECK(f.state == LAMP_ON && f.level == 16);
    CHECK(act & LAMP_ACT_FADE_STOP);
    CHECK(!(act & LAMP_ACT_OFF_TIMER_START));          /* timer = 0 -> không hẹn tắt */

    act = lamp_fsm_handle(&f, LAMP_IN_LIGHT, &s);
    CHECK(f.state == LAMP_FADE_OUT);
    CHECK((act & LAMP_ACT_FADE_START) && (act & LAMP_ACT_OFF_TIMER_STOP));
    ticks(&f, &s, 16);
    CHECK(f.state == LAMP_OFF && f.level == 0);
}

static void test_fsm_manual_mode(void)
{
    lamp_fsm_t f; lamp_settings_t s;
    lamp_fsm_init(&f); lamp_settings_default(&s);
    s.auto_on = 0;

    lamp_fsm_handle(&f, LAMP_IN_DARK, &s);
    CHECK(f.state == LAMP_OFF);                         /* auto tắt: cảm biến không tác dụng */
    CHECK(f.dark == true);
    lamp_fsm_handle(&f, LAMP_IN_TOGGLE, &s);
    CHECK(f.state == LAMP_FADE_IN);
    lamp_fsm_handle(&f, LAMP_IN_TOGGLE, &s);            /* đảo giữa chừng */
    CHECK(f.state == LAMP_FADE_OUT);
    lamp_fsm_handle(&f, LAMP_IN_ON_REQ, &s);
    CHECK(f.state == LAMP_FADE_IN);
    lamp_fsm_handle(&f, LAMP_IN_ON_REQ, &s);            /* bật khi đang bật: không đổi */
    CHECK(f.state == LAMP_FADE_IN);
}

static void test_fsm_off_timer(void)
{
    lamp_fsm_t f; lamp_settings_t s;
    lamp_fsm_init(&f); lamp_settings_default(&s);
    s.off_timer_min = 5;

    lamp_fsm_handle(&f, LAMP_IN_ON_REQ, &s);
    uint8_t act = ticks(&f, &s, 16);
    CHECK(f.state == LAMP_ON);
    CHECK(act & LAMP_ACT_OFF_TIMER_START);
    lamp_fsm_handle(&f, LAMP_IN_OFF_TIMEOUT, &s);
    CHECK(f.state == LAMP_FADE_OUT);
    lamp_fsm_handle(&f, LAMP_IN_OFF_TIMEOUT, &s);       /* timeout thừa: bỏ qua */
    CHECK(f.state == LAMP_FADE_OUT);
}

static void test_fsm_settings_change(void)
{
    lamp_fsm_t f; lamp_settings_t s;
    lamp_fsm_init(&f); lamp_settings_default(&s);
    s.max_level = 8;
    lamp_fsm_handle(&f, LAMP_IN_ON_REQ, &s);
    ticks(&f, &s, 8);
    CHECK(f.state == LAMP_ON && f.level == 8);

    s.max_level = 12;                                   /* tăng max khi đang ON -> sáng dần tiếp */
    lamp_fsm_handle(&f, LAMP_IN_SETTINGS, &s);
    CHECK(f.state == LAMP_FADE_IN);
    ticks(&f, &s, 4);
    CHECK(f.state == LAMP_ON && f.level == 12);

    s.max_level = 4;                                    /* giảm max -> kẹp ngay */
    lamp_fsm_handle(&f, LAMP_IN_SETTINGS, &s);
    CHECK(f.state == LAMP_ON && f.level == 4);

    /* Bật auto khi trời đang tối và đèn đang tắt -> tự sáng */
    lamp_fsm_t g; lamp_settings_t s2;
    lamp_fsm_init(&g); lamp_settings_default(&s2);
    s2.auto_on = 0;
    lamp_fsm_handle(&g, LAMP_IN_DARK, &s2);
    s2.auto_on = 1;
    lamp_fsm_handle(&g, LAMP_IN_SETTINGS, &s2);
    CHECK(g.state == LAMP_FADE_IN);
}

static void test_settings_sanitize(void)
{
    lamp_settings_t s = { 9, 0, 250, 7 };
    lamp_settings_sanitize(&s);
    CHECK(s.mode == LAMP_MODE_FULL);
    CHECK(s.max_level == 1);
    CHECK(s.off_timer_min == LAMP_OFF_TIMER_MAX);
    CHECK(s.auto_on == 1);
}

static void test_menu(void)
{
    menu_t m; lamp_settings_t s;
    menu_init(&m); lamp_settings_default(&s);

    CHECK(menu_handle(&m, MENU_KEY_OK, &s) == MENU_RES_TOGGLE_LAMP);   /* menu đóng: OK = bật/tắt */
    menu_handle(&m, MENU_KEY_NEXT, &s);
    CHECK(m.state == MENU_NAV && m.item == MENU_ITEM_MODE);

    menu_handle(&m, MENU_KEY_OK, &s);                    /* sửa "Che do" */
    CHECK(m.state == MENU_EDIT);
    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* FULL -> BREATH */
    CHECK(s.mode == LAMP_MODE_FULL);                     /* chưa lưu */
    uint8_t r = menu_handle(&m, MENU_KEY_OK, &s);
    CHECK(r & MENU_RES_SETTINGS);
    CHECK(s.mode == LAMP_MODE_BREATH && m.state == MENU_NAV);

    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* sang "Do sang" */
    menu_handle(&m, MENU_KEY_OK, &s);
    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* 16 -> 2 */
    menu_handle(&m, MENU_KEY_OK_HOLD, &s);               /* nhấn giữ = hủy */
    CHECK(s.max_level == 16 && m.state == MENU_NAV);

    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* "Hen tat" */
    menu_handle(&m, MENU_KEY_OK, &s);
    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* 0 -> 1 */
    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* 1 -> 5 */
    menu_handle(&m, MENU_KEY_OK, &s);
    CHECK(s.off_timer_min == 5);

    char buf[12];
    menu_item_value(MENU_ITEM_TIMER, &s, buf, sizeof(buf));
    CHECK(strcmp(buf, "5 ph") == 0);
    menu_item_value(MENU_ITEM_MODE, &s, buf, sizeof(buf));
    CHECK(strcmp(buf, "BREATH") == 0);

    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* "Tu dong" */
    menu_handle(&m, MENU_KEY_NEXT, &s);                  /* "Thoat" */
    CHECK(m.item == MENU_ITEM_EXIT);
    menu_handle(&m, MENU_KEY_OK, &s);
    CHECK(m.state == MENU_CLOSED);
}

static void test_effects(void)
{
    uint8_t rows[8];
    effects_render(LAMP_MODE_FULL, 0, 0, rows);
    CHECK(rows[0] == 0xFF && rows[7] == 0xFF);
    CHECK(effects_intensity(LAMP_MODE_FULL, 123, 16) == 15);
    CHECK(effects_intensity(LAMP_MODE_FULL, 0, 1) == 0);
    CHECK(effects_intensity(LAMP_MODE_BREATH, 0, 16) == 0);    /* đáy chu kỳ thở */
    CHECK(effects_intensity(LAMP_MODE_BREATH, 31, 16) == 15);  /* đỉnh chu kỳ thở */
    CHECK(effects_intensity(LAMP_MODE_BREATH, 31, 9) == 8);    /* đỉnh không vượt level */
    effects_render(LAMP_MODE_WAVE, 0, 0, rows);
    CHECK(rows[7] == 0xFF);                                   /* hàng đáy luôn sáng (h >= 1) */
    uint8_t rows2[8];
    effects_render(LAMP_MODE_WAVE, 3, 0, rows2);
    CHECK(memcmp(rows, rows2, 8) != 0);                       /* sóng có dịch chuyển */
}

static void test_xprintf(void)
{
    char b[48];
    xsnprintf(b, sizeof(b), "%d|%u|%x|%X|%s|%c|%%", -42, 42u, 255u, 255u, "ok", 'Z');
    CHECK(strcmp(b, "-42|42|ff|FF|ok|Z|%") == 0);
    xsnprintf(b, sizeof(b), "[%5d][%-5s][%03u]", 7, "ab", 9u);
    CHECK(strcmp(b, "[    7][ab   ][009]") == 0);
    xsnprintf(b, sizeof(b), "%lu", 4000000000ul);
    CHECK(strcmp(b, "4000000000") == 0);
    int n = xsnprintf(b, 6, "abcdefgh");
    CHECK(n == 8 && strcmp(b, "abcde") == 0);                 /* cắt chuỗi an toàn */
}

int main(void)
{
    printf("Unit test (lamp_fsm, menu, effects, xprintf)\n");
    test_fsm_auto_cycle();
    test_fsm_manual_mode();
    test_fsm_off_timer();
    test_fsm_settings_change();
    test_settings_sanitize();
    test_menu();
    test_effects();
    test_xprintf();
    printf("%d/%d kiem tra dat\n", s_run - s_fail, s_run);
    return s_fail == 0 ? 0 : 1;
}
