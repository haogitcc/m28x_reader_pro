#ifndef __MODULE_CLIENT_H__
#define __MODULE_CLIENT_H__


int socket_connect();

void socket_lister();
void socket_close();

int socket_receive( int timeouts);
int socket_send(char *str, int len);
int socket_send_begin();
int socket_send_end();
void client_start();

#endif//__NET_INCLUDE_H__

