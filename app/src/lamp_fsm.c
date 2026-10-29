/**
 * @file    lamp_fsm.c
 * @brief   [APPLICATION] Cài đặt máy trạng thái đèn: OFF -> FADE_IN -> ON -> FADE_OUT -> OFF.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#include "lamp_fsm.h"

void lamp_settings_default(lamp_settings_t *s)
{
    s->mode = LAMP_MODE_FULL;
    s->max_level = LAMP_LEVEL_MAX;
    s->off_timer_min = 0;
    s->auto_on = 1;
}

void lamp_settings_sanitize(lamp_settings_t *s)
{
    if (s->mode >= LAMP_MODE_COUNT)          { s->mode = LAMP_MODE_FULL; }
    if (s->max_level < 1U)                   { s->max_level = 1U; }
    if (s->max_level > LAMP_LEVEL_MAX)       { s->max_level = LAMP_LEVEL_MAX; }
    if (s->off_timer_min > LAMP_OFF_TIMER_MAX) { s->off_timer_min = LAMP_OFF_TIMER_MAX; }
    s->auto_on = (s->auto_on != 0U) ? 1U : 0U;
}

void lamp_fsm_init(lamp_fsm_t *f)
{
    f->state = LAMP_OFF;
    f->level = 0;
    f->dark = false;
}

/* Chuyển sang sáng dần (nếu đang tắt hoặc đang tối dần) */
static uint8_t go_up(lamp_fsm_t *f)
{
    if (f->state == LAMP_OFF || f->state == LAMP_FADE_OUT) {
        f->state = LAMP_FADE_IN;
        return LAMP_ACT_FADE_START;
    }
    return LAMP_ACT_NONE;
}

/* Chuyển sang tối dần (nếu đang sáng hoặc đang sáng dần) */
static uint8_t go_down(lamp_fsm_t *f)
{
    if (f->state == LAMP_ON || f->state == LAMP_FADE_IN) {
        f->state = LAMP_FADE_OUT;
        return LAMP_ACT_FADE_START | LAMP_ACT_OFF_TIMER_STOP;
    }
    return LAMP_ACT_NONE;
}

static uint8_t enter_on(lamp_fsm_t *f, const lamp_settings_t *s)
{
    f->state = LAMP_ON;
    f->level = s->max_level;
    uint8_t act = LAMP_ACT_FADE_STOP | LAMP_ACT_OFF_TIMER_STOP;
    if (s->off_timer_min > 0U) {
        act |= LAMP_ACT_OFF_TIMER_START;
    }
    return act;
}

uint8_t lamp_fsm_handle(lamp_fsm_t *f, lamp_input_t in, const lamp_settings_t *s)
{
    uint8_t act = LAMP_ACT_NONE;

    switch (in) {
    case LAMP_IN_DARK:
        f->dark = true;
        if (s->auto_on != 0U) { act = go_up(f); }
        break;

    case LAMP_IN_LIGHT:
        f->dark = false;
        if (s->auto_on != 0U) { act = go_down(f); }
        break;

    case LAMP_IN_ON_REQ:
        act = go_up(f);
        break;

    case LAMP_IN_OFF_REQ:
        act = go_down(f);
        break;

    case LAMP_IN_TOGGLE:
        act = (f->state == LAMP_OFF || f->state == LAMP_FADE_OUT) ? go_up(f) : go_down(f);
        break;

    case LAMP_IN_FADE_TICK:
        if (f->state == LAMP_FADE_IN) {
            if (f->level < s->max_level) { f->level++; }
            if (f->level >= s->max_level) { act = enter_on(f, s); }
        } else if (f->state == LAMP_FADE_OUT) {
            if (f->level > 0U) { f->level--; }
            if (f->level == 0U) {
                f->state = LAMP_OFF;
                act = LAMP_ACT_FADE_STOP;
            }
        } else {
            act = LAMP_ACT_FADE_STOP;   /* nhịp thừa (đã ở ON/OFF): dừng timer */
        }
        break;

    case LAMP_IN_OFF_TIMEOUT:
        if (f->state == LAMP_ON) { act = go_down(f); }
        break;

    case LAMP_IN_SETTINGS:
        if (f->state == LAMP_ON) {
            if (f->level < s->max_level) {
                f->state = LAMP_FADE_IN;                 /* max tăng: sáng dần lên mức mới */
                act = LAMP_ACT_FADE_START | LAMP_ACT_OFF_TIMER_STOP;
            } else {
                act = enter_on(f, s);                    /* max giảm/giữ: kẹp level, khởi động lại hẹn giờ */
            }
        } else if (f->state == LAMP_OFF && s->auto_on != 0U && f->dark) {
            act = go_up(f);                              /* vừa bật auto khi trời đang tối */
        }
        break;

    default:
        break;
    }
    return act;
}

const char *lamp_state_name(lamp_state_t st)
{
    switch (st) {
    case LAMP_OFF:      return "OFF";
    case LAMP_FADE_IN:  return "FADE_IN";
    case LAMP_ON:       return "ON";
    case LAMP_FADE_OUT: return "FADE_OUT";
    default:            return "?";
    }
}

const char *lamp_mode_name(uint8_t mode)
{
    switch (mode) {
    case LAMP_MODE_FULL:   return "FULL";
    case LAMP_MODE_BREATH: return "BREATH";
    case LAMP_MODE_WAVE:   return "WAVE";
    default:               return "?";
    }
}
