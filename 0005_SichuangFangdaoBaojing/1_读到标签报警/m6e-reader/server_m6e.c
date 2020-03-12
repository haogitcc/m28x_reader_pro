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


#include "mid_mutex.h"
#include "server_m6e.h"
#include "tmr_utils.h"
#include "tmr_params.h"
#include "serial_reader_imp.h"

#define PORT 8086
#define MSGLEN 256
#define TMR_SR_MAX_PACKET_SIZE 256
#define FILENAME "/root/upgrade_temp.bin"


static int server_fd = -1;
static int client_fd = -1;
static int continueRead = 0;
static int isGpio = -1;

struct M6E_CONFIGURATION {
	int region;
	int readpower;
	int writepower;
	int blf;
	int tari;
	int session;
	int targencoding;
	int target;
};

static struct M6E_CONFIGURATION configuration;

static void*m6e_pthread_send(int fd);

typedef struct tagSend {
	tagEPC *tagQueue;
}tagSend;

sem_t tag_length;
static mid_mutex_t g_mutex = NULL;
//tagEPC *head = NULL;
tagSend *head;

tagEPC * 
dequeue_p()
{
  tagEPC *tagRead;
  pthread_mutex_lock(g_mutex);
  tagRead = head->tagQueue;
  head->tagQueue= head->tagQueue->next;
  pthread_mutex_unlock(g_mutex);
  return tagRead;
}


void enqueue_p(char *epcStr, int len)
{
  pthread_mutex_lock(g_mutex);
  tagEPC *tagStr = (tagEPC *)malloc(sizeof(tagEPC));
  strcpy(tagStr->epc,epcStr);
  tagStr->length = len;
  tagStr->next = head->tagQueue;
  head->tagQueue = tagStr;
  pthread_mutex_unlock(g_mutex);
  sem_post(&tag_length);
}

int pthread_tag_init()
{
      g_mutex = mid_mutex_create();
      if(g_mutex == NULL) {
         printf("mid_mutex_create");
         return -1;
      }

      sem_init(&tag_length, 0, 0);
	  
	  head = (tagSend *)malloc(sizeof(tagSend));
	  return 0;	  
}

void gpio_status_get(int *value)
{
	pthread_mutex_lock(g_mutex);
	*value = isGpio;
	pthread_mutex_unlock(g_mutex);
	return 0;
}

void gpio_status_set(int value)
{
	pthread_mutex_lock(g_mutex);
	if(isGpio != value)
		isGpio = value;
	pthread_mutex_unlock(g_mutex);
	return 0;
}

 int
tcp_sendBytes(uint32_t length, uint8_t* message)
{
	if(client_fd< -1)
		return -1;
	int ret = -1;
	
  	do 
  	{
		ret = send(client_fd, message, length, MSG_NOSIGNAL | MSG_DONTWAIT);
    	if (ret < 0)
    	{
      		return -1;
    	}
    	length -= ret;
    	message += ret;
  	}
 	 while (length > 0);

  	return 0;
}


void m6e_readpower_response(int flags)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	uint16_t crc;
	int ret;
	
	if(0 == flags) {
		int readpower;
		sysSettingGetInt("readpower", &readpower, 1);
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, 0x03);
		SETU8(msg, i, 0x62);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		SETU16(msg,i, readpower);
		crc = tm_crc(&msg[1] , 7);
		msg[8] = crc >> 8;
    	msg[9] = crc & 0xff;
		ret = tcp_sendBytes(10, msg);
	}else if(1 == flags) {
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x92);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		crc = tm_crc(&msg[1] , 4);
		msg[5] = crc >> 8;
    	msg[6] = crc & 0xff;
		ret = tcp_sendBytes(7, msg);
	}

	return ret;
}

void m6e_readpower_get()
{
	m6e_readpower_response(0);
	return;
}

void m6e_readpower_set(uint8_t *message)
{
	int i = 3;
	uint16_t readpower = GETU16(message,i);
	printf("m6e_readpower_set = %d\n",readpower);
	sysSettingSetInt("readpower", readpower & 0xFFFF);
	m6e_readpower_response(1);
	sys_config_save();
	return;
}

void reader_restart()
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	int error,len;
	uint16_t crc;
	
	i = 0;	
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x80);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x00);
	crc = tm_crc(&msg[1] , 4);
	msg[5] = crc >> 8;
    msg[6] = crc & 0xff;
	
	error = tcp_sendBytes(7, msg);
	m6e_shutdown();
	system("reboot");
	return error;	
}

int m6e_upgrade_response(int fd)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	int error,len;
	uint16_t crc;
	
	i = 0;	
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x0D);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x00);
	crc = tm_crc(&msg[1] , 4);
	msg[5] = crc >> 8;
      msg[6] = crc & 0xff;
	
	error = tcp_sendBytes(7, msg);
	return error;
}

static const char *hexChars = "0123456789abcdefABCDEF";

int m6e_filter_reponse(int flags)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	uint16_t crc;
	int ret;
	char filter[64];
	uint8_t *epc;
	
	if(0 == flags) {
		int readpower;
		sysSettingGetString("filter", filter, 64, 1);
	       int len = (int)(strspn(filter, hexChars) / 2);
             epc = malloc(len);
		TMR_hexToBytes(filter, epc, len, NULL);
		printf("filter length: %d , %d\n",strlen(filter), strlen(epc));
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, len);
		SETU8(msg, i, 0xB0);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		memcpy(&msg[i], epc, len);
		i += len;
		
		crc = tm_crc(&msg[1] , i -1);
		msg[i] = crc >> 8;
    		msg[i+1] = crc & 0xff;
		ret = tcp_sendBytes(i+2, msg);
	}else if(1 == flags) {
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0xA0);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		crc = tm_crc(&msg[1] , 4);
		msg[5] = crc >> 8;
    		msg[6] = crc & 0xff;
		ret = tcp_sendBytes(7, msg);
	}

	return ret;	
}

int m6e_filter_set(char *buffer)
{
	char filter[64];
	char epcStr[64];
	int i = 3;
	uint16_t len = GETU8(buffer,i);
	strncpy(filter, buffer + 4, len);
	TMR_bytesToHex(filter, len, epcStr);
	sysSettingSetString("filter", epcStr);
	m6e_filter_reponse(1);
	sys_config_save();
	return;		
}

int m6e_filter_get()
{
	m6e_filter_reponse(0);
	return;
}

int m6e_length_reponse(int flags)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	uint16_t crc;
	int ret;
	
	if(0 == flags) {
		int length;
		sysSettingGetInt("epc_length", &length, 1);
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, 0x03);
		SETU8(msg, i, 0xB1);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		SETU16(msg,i, length);
		crc = tm_crc(&msg[1] , 7);
		msg[8] = crc >> 8;
    		msg[9]= crc & 0xff;
		ret = tcp_sendBytes(10, msg);
	}else if(1 == flags) {
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0xA1);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		crc = tm_crc(&msg[1] , 4);
		msg[5] = crc >> 8;
    		msg[6] = crc & 0xff;
		ret = tcp_sendBytes(7, msg);
	}

	return ret;
}

int m6e_id_reponse(int flags)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	uint16_t crc;
	int ret;
	char filter[64];
	uint8_t *epc;
	
	if(0 == flags) {
		int readpower;
		sysSettingGetString("id", filter, 64, 1);
	       int len = (int)(strspn(filter, hexChars) / 2);
             epc = malloc(len);
		TMR_hexToBytes(filter, epc, len, NULL);
		printf("filter length: %d , %d\n",strlen(filter), strlen(epc));
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, len);
		SETU8(msg, i, 0xB2);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		memcpy(&msg[i], epc, len);
		i += len;
		
		crc = tm_crc(&msg[1] , i -1);
		msg[i] = crc >> 8;
    		msg[i+1] = crc & 0xff;
		ret = tcp_sendBytes(i+2, msg);
	}else if(1 == flags) {
		i = 0;	
		SETU8(msg, i, 0xFF);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0xA2);
		SETU8(msg, i, 0x00);
		SETU8(msg, i, 0x00);
		crc = tm_crc(&msg[1] , 4);
		msg[5] = crc >> 8;
    		msg[6] = crc & 0xff;
		ret = tcp_sendBytes(7, msg);
	}

	return ret;	
}

int m6e_id_set(char *buffer)
{
	char id[64];
	char epcStr[64];
	int i = 3;
	uint16_t len = GETU8(buffer,i);
	strncpy(id, buffer + 4, len);
	TMR_bytesToHex(id, len, epcStr);
	sysSettingSetString("id", epcStr);
	m6e_id_reponse(1);
	sys_config_save();
	return;
}

int m6e_id_get()
{	
	m6e_id_reponse(0);
	return;
}

int m6e_length_set(char *buffer)
{
	int i = 3;
	uint16_t len = GETU16(buffer,i);
	sysSettingSetInt("epc_length", len);
	m6e_length_reponse(1);
	sys_config_save();
	return;
}

int m6e_length_get()
{	
	m6e_length_reponse(0);
	return;
}

int m6e_read_data(char *buffer )
{
	int flags = (int)buffer[3];
	if(flags) {
		
	} else {

	}
}

static int
tcp_receiveBytes(int fd, uint32_t length, uint32_t* messageLength, uint8_t* message, const uint32_t timeoutMs)
{

	if(client_fd < -1)
		return -1;
  	int ret;
	int len,error;
  	struct timeval tv;
  	fd_set set;
  	int status = 0;

  	*messageLength = 0;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    tv.tv_sec = timeoutMs/1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    	/* Ideally should reset this timeout value every time through */
    ret = select(fd + 1, &set, NULL, NULL, &tv);
    if (ret < 1)
    {
      	return -1;
    }

	if(fd != client_fd) {
		printf("other client connected!\n");
		return -1;
	}

    if(FD_ISSET(fd, &set)) {
        ret = -1;
        len = sizeof(ret);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&ret, &len);

        if(ret != 0) {
            printf("%s,client's socket timeout!!!\n", strerror(errno));
            close(fd);
            client_fd = -1;
            return -1;
        }

		len = 0;
		while(len < MSGLEN && (error = recv(fd, messageLength + len, MSGLEN - len, MSG_DONTWAIT)) > 0)
			len += error;
        }

  	return 0;
}

int anetKeepAlive(int fd, int interval)
{
	int val = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0)
		printf("setsockopt SO_KEEPALIVE error!!!\n");

	val = interval;
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0)
		printf("setsockopt TCP_KEEPIDLE error!!!\n");

	val = 3;
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0)
		printf("setsockopt TCP_KEEPINTVL error!!!\n");	
	
	val = 2;
	if(setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0)
		printf("setsockopt TCP_KEEPCNT error!!!\n");

	printf("anetKeepAlive success!\n");
	return 0;

}


static void*m6e_pthread_client(void *arg)
{
	int ret = -1;
	tagEPC *tagRead;
	int length = 0;
	int connected_fd = -1;
	int len,error;
  	struct timeval tv;
	int timeoutMs = 3000;
  	fd_set set,errornew_fd;
	int ratebund;
	int opcode;

	if(client_fd < 0) {
		printf("Client fd is error %d\n", client_fd);
		close(client_fd);
		client_fd = -1;
		return NULL;
	}

	char *buffer = (char *)malloc(256);
	
	connected_fd = client_fd;
	if(continueRead)
		sleep(2);
	while(1) {
    	FD_ZERO(&set);
		FD_ZERO(&errornew_fd);
    	FD_SET(connected_fd, &set);
		FD_SET(connected_fd, &errornew_fd);
    	tv.tv_sec = timeoutMs/1000;
    	tv.tv_usec = (timeoutMs % 1000) * 1000;
    	/* Ideally should reset this timeout value every time through */
    	ret = select(connected_fd + 1, &set, NULL, &errornew_fd, NULL);
		//printf("select ret = %d\n",ret);
    	if (ret < 1 || connected_fd != client_fd)
    	{
    		printf("select error!\n");
      		break;
    	}

		if(0 == ret)
			continue;

        if(FD_ISSET(connected_fd, &set) || FD_ISSET(connected_fd, &errornew_fd)) {
            ret = -1;
            len = sizeof(ret);
            getsockopt(connected_fd, SOL_SOCKET, SO_ERROR, (void*)&ret, &len);

            if(ret != 0) {
                printf("%s,client's socket timeout!!!\n", strerror(errno));
                close(connected_fd);
                connected_fd = -1;
                break;
            }

			len = 0;
			memset(buffer,0,MSGLEN);
			error = recv(connected_fd, buffer, 256, MSG_DONTWAIT);
			//while(len < MSGLEN && ((len = recv(connected_fd, buffer + len, MSGLEN - len, 0)) > 0))
			//	len += error;
			
			printf("buffer = %p,%p,%p,%p,%p, %p,%d\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],error);
			if(error == 0)
				break;
			
			if(buffer[0] == 0xFF && buffer[1] == 0xFF)
				continue;
			if((buffer[1]+5) != error)
				error = buffer[1] + 5;

			if(buffer[2] == 0x80)  //ÖØÆô¶ÁÐ´Æ÷
				reader_restart();
			else if(buffer[2] == 0x92)
				m6e_readpower_set(buffer);
			else if(buffer[2] == 0x62)
				m6e_readpower_get();
			else if(buffer[2] == 0xA0) 
				m6e_filter_set(buffer);
			else if(buffer[2] == 0xB0)
				m6e_filter_get();
			else if(buffer[2] == 0xA1) 
				m6e_length_set(buffer);
			else if(buffer[2] == 0xB1)
				m6e_length_get();
			else if(buffer[2] == 0xA2) 
				m6e_id_set(buffer);
			else if(buffer[2] == 0xB2)
				m6e_id_get();
			//else if(buffer[2] == 0x29)
			//	m6e_read_data(buffer);
			//m6e_set_opcode(buffer);
			else if(buffer[2] == 0x0D)
			{
				if(m6e_upgrade_openfile())
					continue;
				error = m6e_upgrade_resolve(buffer);
				if(error > 0) {
				    int flags = m6e_upgrade_response(connected_fd);
				}
				if(240 == error)
					continue;
				else {
					m6e_upgrade_close();
					if(m6e_upgrade_check() < 0)
						continue;
					m6e_upgrade_reboot();
				}
			}			
        }		
	}	
	printf("end!\n");
	m6e_shutdown();
	free(buffer);
	return NULL;
}

int m6e_start(void)
{
    //int server_fd = -1; // sendlen;
    struct sockaddr_in tcp_addr;
    int len = 0;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        printf("Server sockect creat\n");
        return -1;
    }

    int opt = 1;
    len = sizeof(opt);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, len);

    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(PORT);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(server_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        printf("Server socket bind error\n");
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    if(listen(server_fd, 1) < 0) {
        printf("Server socket listen error\n");
        close(server_fd);
        server_fd = -1;
        return -1;
    }

    while(1) {
        len = sizeof(tcp_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&tcp_addr, &len);

        if(client_fd < 0) {
            printf("Server accept error\n");
			perror("accept:");
            if (EBADF == errno)
                break;
            continue;
        }

        printf("Server accept success\n");

		anetKeepAlive(client_fd, 10);
		int sendbuf = 4096;
		setsockopt(client_fd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));
        pthread_t stbmonitor_pthread = 0;
        pthread_attr_t tattr;
        pthread_attr_init(&tattr);
        pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
        pthread_create(&stbmonitor_pthread, &tattr, m6e_pthread_client, NULL);
    }
    return 0;
}


void m6e_shutdown()
{
    if (client_fd > 0) {
        close(client_fd);
        client_fd = -1;
    }
}


