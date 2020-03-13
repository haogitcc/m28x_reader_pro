#ifndef _CLIENT_DATA_H
#define _CLIENT_DATA_H

#ifdef  __cplusplus
extern "C" {
#endif


int connect_server_data();
int socket_receive_ex( int timeouts, int fd);
int socket_send_ex(char *str, int len, int fd);
void socket_data_close(int fd);


#ifdef __cplusplus
}
#endif

#endif

