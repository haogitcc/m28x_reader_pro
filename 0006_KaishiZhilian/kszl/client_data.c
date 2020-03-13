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

int connect_server_data()
{
	struct sockaddr_in server;
	fd_set fdr, fdw;
	int socket_fd;
	struct timeval timeout;
	int port;
	char ip[64];
	int flags, res;
	
	sysSettingGetString("serverip", ip, 64, 1);
	sysSettingGetInt("serverport", &port, 1);
	printf("connect data server\n");
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("socket error!\n");
		goto END;
	}

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(ip);

	if((flags = fcntl(socket_fd, F_GETFL, 0)) < 0)
	{
		perror("Network test...\n");
		goto END;
	}

	if( fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		perror("Network test...\n");
		goto END;
	}

	if(connect(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		if(errno != EINPROGRESS) {
			perror("Network test...\n");
			goto END;
		}	
	

		FD_ZERO(&fdr);
		FD_ZERO(&fdw);
		FD_SET(socket_fd, &fdr);
		FD_SET(socket_fd, &fdw);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;

		res = select(socket_fd + 1, &fdr, &fdw, NULL, &timeout);
		if(res <= 0) {
			perror("Network test...\n");
			goto END;
		}

		if(!FD_ISSET(socket_fd, &fdw))
		{
			printf("no events on sockfd found\n\n");
			goto END;
		}

      		int error = 0;
      		socklen_t length = sizeof( error );
      		if( getsockopt( socket_fd, SOL_SOCKET, SO_ERROR, &error, &length ) < 0 )
      		{
         		 printf( "get socket option failed\n" );
			goto END;
     		 }
 
     		 if( error != 0 )
      		{
          		printf( "connection failed after select with the error: %d \n", error );
			perror("error:\n");
			goto END;
      		}
	}
	printf("connect data server success\n");

	return socket_fd;

END:
	close(socket_fd);
	return -1;
}

int socket_receive_ex( int timeouts, int fd)
{
	int ret = -1, status = 0;
	fd_set set;
	struct timeval tv;
	char message[1024];
	
	FD_ZERO(&set);
	FD_SET(fd, &set);
	tv.tv_sec = timeouts/1000;
	tv.tv_usec = (timeouts%1000)  * 1000;

	ret = select(fd+1, &set, NULL, NULL, &tv);
	if(ret < 1)
	{
		printf("receive timeout\n");
		return -1;
	}

	ret = recv(fd, message, 1024, MSG_DONTWAIT);
	if(0 == ret || ret < 10 || message[0] != '<')
	{
		printf("receive failed\n");
		return -1;
	}
	printf("socket_receive_ex : %s\n", message);
	
	return parse_command(message);	
}

int socket_send_ex(char *str, int len, int fd)
{
	int ret ;
	if(fd < 0) {
		printf("reader connect failed\n");
		return -1;
	}
	do
	{
		ret = write(fd, str, len);
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
	//printf("send success\n");
	return 0;
}

