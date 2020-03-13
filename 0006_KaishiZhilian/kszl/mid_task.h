#ifndef _MID_TASK_H__2011_03_11
#define _MID_TASK_H__2011_03_11


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mid_func_t)(void *arg);

void mid_task_init(void);
int mid_task_create(const char* name, mid_func_t func, void *arg);
int mid_task_create_ex(const char* name, mid_func_t func, void *arg);
void mid_task_delay(unsigned int ms);

#ifdef __cplusplus
}
#endif


#endif /* _MID_OS_TASK_H_2007_3_24__ */
