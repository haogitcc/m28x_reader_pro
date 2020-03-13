#ifndef _TMR_FILE_H_
#define _TMR_FILE_H_

#ifdef  __cplusplus
extern "C" {
#endif

int tmr_file_init();
int tmr_file_write(char *str, int number);
int tmr_file_read(int fd);

#ifdef __cplusplus
}
#endif

#endif
