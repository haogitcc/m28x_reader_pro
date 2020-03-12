#ifndef __MID_TIMER_H__
#define __MID_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL             0L
#endif


typedef void (*mid_timer_f)(int arg);

void mid_timer_init(void);

int mid_timer_create(int intval, int loops, mid_timer_f func, int arg);
void mid_timer_delete(mid_timer_f func, int arg);
void mid_timer_delete_all(mid_timer_f func);
void mid_timer_print(void);

#ifdef __cplusplus
}
#endif


#endif /* __MID_TIMER_H__ */
