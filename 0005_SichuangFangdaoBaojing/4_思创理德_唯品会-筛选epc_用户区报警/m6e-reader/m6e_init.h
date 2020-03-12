#ifndef _M6E_INIT_H
#define _M6E_INIT_H

#ifdef  __cplusplus
extern "C" {
#endif

int m6e_init(char * string);
int set_bundrate(int rate);
int m6e_destory();
int m6e_set_configuration(int key, int value);

#ifdef __cplusplus
}
#endif



#endif

