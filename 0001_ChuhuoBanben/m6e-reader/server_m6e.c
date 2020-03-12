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

#define PORT 8086
#define MSGLEN 256
#define TMR_SR_MAX_PACKET_SIZE 256
#define FILENAME "/root/upgrade_temp.bin"


static int server_fd = -1;
static int client_fd = -1;
static int continueRead = 0;

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

static uint16_t crctable[] = 
{
  0x0000, 0x1021, 0x2042, 0x3063,
  0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b,
  0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};


static uint16_t
tm_crc(uint8_t *u8Buf, uint8_t len)
{
  uint16_t crc;
  int i;

  crc = 0xffff;

  for (i = 0; i < len ; i++)
  {
    crc = ((crc << 4) | (u8Buf[i] >> 4))  ^ crctable[crc >> 12];
    crc = ((crc << 4) | (u8Buf[i] & 0xf)) ^ crctable[crc >> 12];
  }

  return crc;
}

int m6e_readpower_Resolve(char *value)
{
	configuration.readpower = GETU16AT(value,3);
	sysSettingSetInt("readpower", configuration.readpower);
	return 0;
}

int m6e_writepower_Resolve(char *value)
{
	configuration.writepower = GETU16AT(value,3);
	sysSettingSetInt("writepower", configuration.writepower);
	return 0;
}

int m6e_region_Resolve(char *value)
{
	configuration.region = GETU8AT(value,3);
	sysSettingSetInt("region", configuration.region);
	return 0;
}

int m6e_blf(int value)
{
	switch(value) {
		case 0x03:
			configuration.blf = 40;
			break;
		case 0x00:
			configuration.blf = 250;
			break;
		case 0x02:
			configuration.blf = 320;
			break;
		case 0x04:
			configuration.blf = 640;
	}
	return 0;
}

int m6e_target(int value)
{
	switch(value) {
		case 0x0100:
			configuration.target = 0;
			break;
		case 0x0101:
			configuration.target = 1;
			break;
		case 0x0000:
			configuration.target = 2;
			break;
		case 0x0001:
			configuration.target = 3;
			break;
		default:
			break;
	}
	return 0;
}

int m6e_set_configuration_v(char *value)
{
	uint16_t taget;
	switch(value[4]) {
		case 0x00:
			configuration.session = GETU8AT(value,5);
			sysSettingSetInt("session", configuration.session);
			break;
		case 0x01:
			taget = GETU16AT(value,5);
			m6e_target(taget);
			sysSettingSetInt("target", configuration.target);
			break;
		case 0x02:
			configuration.targencoding = GETU8AT(value,5);
			sysSettingSetInt("targencoding", configuration.targencoding);
			break;
		case 0x10:
			taget = GETU8AT(value,5);
			m6e_blf(taget);
			sysSettingSetInt("blf", configuration.blf);
			break;
		case 0x11:
			configuration.tari = GETU8AT(value,5);
			sysSettingSetInt("tari", configuration.tari);
			break;
		case 0x12:
			break;
	}
	return 0;
}


int m6e_set_opcode(char *value)
{
	switch(value[2]) {
		case 0x9B:
			m6e_set_configuration_v(value);
			break;
		case 0x92:
			m6e_readpower_Resolve(value);
			break;
		case 0x94:
			m6e_writepower_Resolve(value);
			break;
		case 0x97:
			m6e_region_Resolve(value);
			break;
		
		default:
			return;
	}
	sys_config_timer();
}

int m6e_configuration_init()
{
	int ret = -1;
	sysSettingGetInt("region", &configuration.region, 0);
	ret = m6e_set_configuration(TMR_PARAM_REGION_ID, configuration.region);
	
	sysSettingGetInt("readpower", &configuration.readpower, 0);
	ret = m6e_set_configuration(TMR_PARAM_RADIO_READPOWER, configuration.readpower);
	
	sysSettingGetInt("writepower", &configuration.writepower, 0);
	ret = m6e_set_configuration(TMR_PARAM_RADIO_WRITEPOWER, configuration.writepower);
	
	sysSettingGetInt("blf", &configuration.blf, 0);
	ret = m6e_set_configuration(TMR_PARAM_GEN2_BLF, configuration.blf);
	
	sysSettingGetInt("session", &configuration.session, 0);
	ret = m6e_set_configuration(TMR_PARAM_GEN2_SESSION, configuration.session);
	
	sysSettingGetInt("targencoding", &configuration.targencoding, 0);
	ret = m6e_set_configuration(TMR_PARAM_GEN2_TAGENCODING, configuration.targencoding);
	
	sysSettingGetInt("tari", &configuration.tari, 0);
	ret = m6e_set_configuration(TMR_PARAM_GEN2_TARI, configuration.tari);
	
	sysSettingGetInt("target", &configuration.target, 0);
	ret = m6e_set_configuration(TMR_PARAM_GEN2_TARGET, configuration.target);
	return 0;
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

int m6e_baudrate(int rate)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	int error,len;
	uint16_t crc;
	
	i = 0;	
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x04);
	SETU8(msg, i, 0x06);
	SETU32(msg, i, rate);
	crc = tm_crc(&msg[1] , 6);
	msg[7] = crc >> 8;
    msg[8] = crc & 0xff;
	
	error = serial_sendBytes(9,msg);
	printf("baudrate = %d\n",error);
	error = receiveMessage(msg, &len, 5000);
	if((error == 0) && (0 == GETU16AT(msg, 3)))
		serial_setBaudRate(rate);
}


int m6e_version()
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	int error,len;
	uint16_t crc;
	
	i = 0;	
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x0c);
	crc = tm_crc(&msg[1] , 2);
	msg[3] = crc >> 8;
    msg[4] = crc & 0xff;
	
	error = serial_sendBytes(9,msg);
	printf("m6e_version = %d\n",error);
	error = receiveMessage(msg, &len, 5000);
	printf("cuurent = %p\n",msg[5]);
}

static int
tcp_sendBytes(int fd, uint32_t length, uint8_t* message)
{
	if(fd < -1)
		return -1;
	int ret = -1;
	
  	do 
  	{
		ret = send(fd, message, length, MSG_NOSIGNAL | MSG_DONTWAIT);
		//printf("tcp send : %d\n", ret);
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


int m6e_stop()
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint8_t i;
	int error,len;
	uint16_t crc;
	
	i = 0;	
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x03);
	SETU8(msg, i, 0x2F);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x02);
	crc = tm_crc(&msg[1] , 5);
	msg[6] = crc >> 8;
    	msg[7] = crc & 0xff;
	
	error = serial_sendBytes(9,msg);	
}

int reader_gpi(char *buffer, int fd)
{
	int ret = -1, len, j,i;
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	Fum6_GpioPin state[4], stategpi[3];
	uint8_t offset = 0;
	uint16_t crc;

	ret = serial_sendBytes(buffer[1] +5, buffer);
	ret = receiveMessage(msg, &len, 5000);
	printf("m6e_gpio = %d\n",ret);

 	 len = (msg[1] - 1)/3;
	 
  	offset = 6;
 	 for (j = 0; j < len ; j++)
  	{
    		state[j].id = msg[offset++];
    		state[j].output = (1 == msg[offset++]) ? true : false;
    		state[j].high = (1 == msg[offset++]) ? true : false;
  	}
	memset(msg, 0, TMR_SR_MAX_PACKET_SIZE);
	 j = 0;
  	for (i = 0 ; i < len ; i++)
  	{
   		 if (!state[i].output)
   		 {
      			/* If pin is input, only then copy to output */
     			 stategpi[j].id = state[i].id;
     		 	stategpi[j].high = state[i].high;
      			stategpi[j].output = state[i].output;
      			j ++;
   		 }
   	}
	stategpi[j].id = 5;
     	stategpi[j].high = (0 ==gpio_read(62)? false:true);
      	stategpi[j].output = state[i].output;

	j ++;
	stategpi[j].id = 6;
     	stategpi[j].high = (0 ==gpio_read(63)? false:true);
      	stategpi[j].output = state[i].output;

	i = 0;	
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x0A);
	SETU8(msg, i, 0x66);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x01);
	for(j = 0; j < 3;j++)
	{
		SETU8(msg, i, stategpi[j].id);
		SETU8(msg, i, stategpi[j].output);
		SETU8(msg, i, stategpi[j].high);
	}

	crc = tm_crc(&msg[1] , 14);
	msg[15] = crc >> 8;
    	msg[16] = crc & 0xff;

	ret = tcp_sendBytes(fd, 17, msg);
	//printf("m6e_gpio = %d, ret = %d \n",ret, strlen(msg));
	//printf("buffer = %p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p %d\n",msg[0],msg[1],msg[2],msg[3],msg[4],msg[6], msg[8],msg[9],msg[11],msg[12],msg[14],ret);

}


void reader_gpo(int fd, int gpo, int value)
{
	uint8_t msg[TMR_SR_MAX_PACKET_SIZE];
	uint16_t crc;

	uint8_t i;
	if(7 == gpo)
		gpio_write(61, value);
	else if(8 == gpo)
	{
		gpio_write(60, value);
	}
	
	i=0;
	SETU8(msg, i, 0xFF);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x96);
	SETU8(msg, i, 0x00);
	SETU8(msg, i, 0x00);
	crc = tm_crc(&msg[1] , 4);
	msg[5] = crc >> 8;
      msg[6] = crc & 0xff;

	tcp_sendBytes(fd, 7, msg);
}

int reader_restart(int fd)
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
	
	error = tcp_sendBytes(fd, 7, msg);
	system("reboot");
	
	return error;	
}


int sqlite3_send(uint32_t length, uint8_t* message)
{
	return tcp_sendBytes(client_fd, length, message);
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
	
	error = tcp_sendBytes(fd, 7, msg);
	return error;
}

int m6e_firmware_response(char *buffer)
{
	uint16_t crc;
	int version1 = 0x1,version2 = 0x1,version3= 0x1,version4 = 0x6B;
	sscanf(FIRMWARE, "%d.%d.%d.%d", &version1, &version2, &version3, &version4);
	buffer[17] = version1;
	buffer[18] = version2;
	buffer[19] = version3;
	buffer[20] = version4;
	crc = tm_crc(&buffer[1], 24);
	buffer[25] = crc >> 8;
	buffer[26] = crc & 0xff;
	return 0;
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


void*m6e_pthread_client(void *arg)
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
		error = recv(connected_fd, buffer, MSGLEN, MSG_DONTWAIT);
		//while(len < MSGLEN && ((len = recv(connected_fd, buffer + len, MSGLEN - len, 0)) > 0))
		//	len += error;
			
		//printf("buffer = %p,%p,%p,%p,%p, %d\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],error);
		if(error == 0)
			break;
			
		if(buffer[0] == 0xFF && buffer[1] == 0xFF)
			continue;
		
		if(buffer[0] != 0xFF || error != buffer[1] + 5)
			continue;
		
		if((buffer[1]+5) != error)
			error = buffer[1] + 5;

		if(buffer[2] == 0x80)  //\D6\D8\C6\F4\B6\C1д\C6\F7
			reader_restart(connected_fd);
		else if(buffer[2] == 0x66) {
			reader_gpi(buffer, connected_fd);
			continue;
		} else if(buffer[2] == 0x96 && (buffer[3] == 0x07 || buffer[3] == 0x08)) {
			reader_gpo(connected_fd, buffer[3], buffer[4]);
			continue;
		}

		m6e_set_opcode(buffer);
		if(buffer[2] == 0x0D)
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

		opcode = buffer[2];
		error = serial_sendBytes(error, buffer);
		//printf("error = %d\n",error);				
try:
		if(continueRead == 0) {
			memset(buffer,0,MSGLEN);
			len = 0;
			error = receiveMessage(buffer, &len, 30000);
			//printf("serial_receiveBytes = %p,%p,%p,%p,%p,%d,%d\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],len,error);
			if(len <= 0)
				continue;
			if(buffer[2] != opcode) {
				printf("opcode does not match!\n");
				goto try;
			}
			if(buffer[2] == 0x03)
				m6e_firmware_response(buffer);
			error = tcp_sendBytes(connected_fd, len, buffer);
				//printf("tcp send = %d\n",error);
		}
		if(!continueRead && (buffer[0]== 0xFF) && (buffer[2] == 0x2F) && (buffer[6] == 0x22) ) {
			printf("continue!\n");
			pthread_t stbmonitor_pthread = 0;
			pthread_attr_t tattr;
			pthread_attr_init(&tattr);
			pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
			pthread_create(&stbmonitor_pthread, &tattr, m6e_pthread_send, connected_fd);
			pthread_detach(stbmonitor_pthread);
			//mid_task_create_ex("send_client", m6e_pthread_send, connected_fd);
				
			//mid_task_create_ex("send_client1", send_client, NULL);
			continueRead = 1;
		}
        }		
	}	
	printf("end!\n");
	m6e_shutdown();
	free(buffer);
	return NULL;
}

void*m6e_pthread_send(int fd)
{
	int ret = -1;
	int len = 0;
	int i = 0;
	int stop = 0;
	char *buffer = (char *)malloc(256);
	while(1)
	{
		if((fd != client_fd) && (0 == stop)) {
			printf("other client connected!\n");
			m6e_stop();
			stop = 2;
		}
		len = 0;
		memset(buffer, 0, 256);
		ret =  receiveMessage(buffer, &len, 5000);
		//printf("serial_receiveBytes = %p,%p,%d\n",buffer[0],buffer[1],len);
		if(0 == ret)
		{
			if(0 == stop){
				ret = tcp_sendBytes(fd, buffer[1] + 7, buffer);
				if(ret < 0) {
					perror("network failed:");
					stop = 1;
					m6e_stop();
				}
			}
		}
		if(continueRead && (buffer[0]== 0xFF)  && (buffer[1] == 0x01) && (buffer[2] == 0x2F) && (buffer[5] == 0x02)) {

			continueRead = 0;
			break;
		}

	}
	printf("send end!\n");
	free(buffer);
	if(2 == stop)
		m6e_shutdown();
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
	int sendbuf = 6144;
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


