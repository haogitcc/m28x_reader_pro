#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <linux/tcp.h>
#include <stdint.h>
#include <fcntl.h>

#include "mid_timer.h"
#include "tmr_command.h"
static int socket_fd = -1;
#define TIMEOUT 5

int socket_connect()
{
	struct sockaddr_in server;
	fd_set fdr, fdw;
	struct timeval timeout;
	int port;
	char ip[64];
	int flags, res;
	printf("connect param server\n");
	sysSettingGetString("serverip", ip, 64, 1);
	sysSettingGetInt("serverport", &port, 1);

	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("socket error!\n");
		return -1;
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port+1);
	server.sin_addr.s_addr = inet_addr(ip);

	if((flags = fcntl(socket_fd, F_GETFL, 0)) < 0)
	{
		perror("Network test...\n");
		socket_close();
		return -1;
	}

	if( fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		perror("Network test...\n");
		socket_close();
		return -1;
	}

	if(connect(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		if(errno != EINPROGRESS) {
			perror("Network test...\n");
			socket_close();
			return -1;
		}	
	} else {
		anetKeepAlive(socket_fd, 10);
		printf("connect to server\n");
		return 0;
	}

	FD_ZERO(&fdr);
	FD_ZERO(&fdw);
	FD_SET(socket_fd, &fdr);
	FD_SET(socket_fd, &fdw);
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

	res = select(socket_fd + 1, &fdr, &fdw, NULL, &timeout);
	if(res <= 0) {
		perror("Network test...\n");
		socket_close();
		return -1;
	}

	if(!FD_ISSET(socket_fd, &fdw))
	{
		printf("no events on sockfd found\n\n");
		socket_close();
		return -1;
	}

      int error = 0;
      socklen_t length = sizeof( error );
      if( getsockopt( socket_fd, SOL_SOCKET, SO_ERROR, &error, &length ) < 0 )
      {
          printf( "get socket option failed\n" );
          socket_close();
          return -1;
      }
 
      if( error != 0 )
      {
          printf( "connection failed after select with the error: %d \n", error );
          socket_close();
          return -1;
      }
	anetKeepAlive(socket_fd, 10);
	printf("connect to server\n");
	return 0;
}


void socket_lister()
{
	int len,error;
  	struct timeval tv;
	int timeoutMs = 2000;
  	fd_set set,errornew_fd;
	int ret, isSend = 0 ,i = 0;
	char *buffer = (char *)malloc(512);

	printf("socket_lister\n");
	while(1)
	{
    		FD_ZERO(&set);
		FD_ZERO(&errornew_fd);
    		FD_SET(socket_fd, &set);
		FD_SET(socket_fd, &errornew_fd);
    		tv.tv_sec = timeoutMs/1000;
    		tv.tv_usec = (timeoutMs % 1000) * 1000;
    		/* Ideally should reset this timeout value every time through */
			printf("socket_lister\n");
	
    		ret = select(socket_fd + 1, &set, NULL, &errornew_fd, &tv);
		//printf("select ret = %d\n",ret);
    		if (ret < 0)
    		{
      			break;
    		}
	
		if(0 == ret) 
			continue;		

        	 if(FD_ISSET(socket_fd, &set) || FD_ISSET(socket_fd, &errornew_fd)) {
            		ret = -1;
            		len = sizeof(ret);
            		getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, (void*)&ret, &len);

            		if(ret != 0) {
                		printf("socket_lister : %s,client's socket timeout!!!\n", strerror(errno));
                		break;
           		}

			len = 0;
			memset(buffer,0,512);
			error = recv(socket_fd, buffer, 512, MSG_DONTWAIT);
			printf("client: %s\n", buffer);
			if(error == 0)
				break;
			parse_command(buffer);
			
        	 }
	}	

	free(buffer);
	socket_close();
	printf("socket shutdown\n");
}

void socket_close()
{
	if(socket_fd > 0) 
	{
		close(socket_fd);
		socket_fd = -1;
	}
}

int socket_receive( int timeouts)
{
	int ret = -1, status = 0;
	fd_set set;
	struct timeval tv;
	char message[1024];
	memset(message, 0, 1024);
	
	FD_ZERO(&set);
	FD_SET(socket_fd, &set);
	tv.tv_sec = timeouts/1000;
	tv.tv_usec = (timeouts%1000)  * 1000;

	ret = select(socket_fd+1, &set, NULL, NULL, &tv);
	if(ret < 1)
	{
		printf("receive timeout\n");
		return -1;
	}

	ret = recv(socket_fd, message, 1024, MSG_DONTWAIT);
	if(0 == ret)
	{
		printf("receive failed\n");
		return -1;
	}
	printf("socket_receive : %s\n",message);
	return parse_command(message);	
}

int socket_send(char *str, int len)
{
	int ret ;
	printf("socket_send: %s , %d\n", str, len);
	if(socket_fd < 0) {
		printf("reader connect failed\n");
		return -1;
	}

	do
	{
		ret = write(socket_fd, str, len);
		if(ret == -1)
		{
			printf("send failed");
			return -1;
		}
		len -= ret;
		str += ret;
	}
	while(len >0);
	//write(socket_fd, "\n", 2);
	printf("send success\n");
	return 0;
}

void client_start()
{
	int ret = -1;

start:
	if(socket_fd < 0) {
		mid_timer_delete((mid_timer_f)get_server_time, NULL);
		if(socket_connect() < 0) {
			sleep(10);
			goto start;
		}
		//每次连接成功后从服务器获取参数
		//get_server_time();
		//get_server_reader();
	}
	mid_timer_create(1, 1, (mid_timer_f) get_server_reader, NULL);
	mid_timer_create(2, 1, (mid_timer_f) get_server_time, NULL);
	socket_lister();
	goto start;
}

