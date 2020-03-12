#ifndef __MID_MUTEX_H_2007_3_25__
#define __MID_MUTEX_H_2007_3_25__

typedef struct mid_mutex* mid_mutex_t;

mid_mutex_t mid_mutex_create(void);
int mid_mutex_lock0(mid_mutex_t mutex,const char* file,const char *fun,int line);
int mid_mutex_unlock(mid_mutex_t mutex);

#define mid_mutex_lock(mutex) mid_mutex_lock0(mutex,__FILE__,__FUNCTION__,__LINE__)

#endif /* __MID_OS_MUTEX_H_2007_3_25__ */
