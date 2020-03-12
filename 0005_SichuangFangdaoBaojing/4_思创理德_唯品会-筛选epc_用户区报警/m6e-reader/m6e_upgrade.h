#ifndef _M6E_UPGRADE_H_

#define _M6E_UPRADE_H_

#ifndef SEEK_SET
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif


int m6e_upgrade_openfile();
int m6e_upgrade_resolve(char *buffer);
int m6e_upgrade_close();

#endif
