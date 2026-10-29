/* [MÔ PHỎNG] bản giả của timer.h */
#ifndef HAL_TIMER_H
#define HAL_TIMER_H
#include <stdint.h>
#include <time.h>
typedef void (*timer_overflow_cb_t)(void);
static inline void tim2_freerun_init(uint32_t hz, uint8_t p, timer_overflow_cb_t cb){(void)hz;(void)p;(void)cb;}
static inline uint32_t sim_us(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return (uint32_t)(t.tv_sec*1000000ull + t.tv_nsec/1000); }
static inline uint16_t tim2_get_count(void){ return 0; }
static inline int tim2_overflow_pending(void){ return 0; }
#define __WFI() do{}while(0)
#define __DSB() do{}while(0)
#endif
