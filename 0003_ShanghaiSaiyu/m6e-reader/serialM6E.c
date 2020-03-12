#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h>

#include "serial_m6e.h"

static int serial_fd;


int serial_open(const char *str)
{
  int ret;
  struct termios t;
  serial_fd = open(str, O_RDWR | O_NOCTTY | O_NDELAY);

  if (serial_fd == -1) {
  	perror("open_port: Unable to open %s",str);
    return -1;
  }
  /*
   * Set 8N1, disable high-bit stripping, soft flow control, and hard
   * flow control (modem lines).
   */
  ret = tcgetattr(serial_fd, &t);
  if (-1 == ret)
    return errno;
  t.c_iflag &= ~(ICRNL | IGNCR | INLCR | INPCK | ISTRIP | IXANY 
                 | IXON | IXOFF | PARMRK);
  t.c_oflag &= ~OPOST;
  t.c_cflag &= ~(CRTSCTS | CSIZE | CSTOPB | PARENB);
  t.c_cflag |= CS8 | CLOCAL | CREAD | HUPCL;
  t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 1;
  ret = tcsetattr(serial_fd, TCSANOW, &t);
  if (-1 == ret)
    return errno;

  return 0;
}

int serial_sendBytes(uint32_t length, uint8_t* message)
{
  int ret;
  do 
  {
    ret = write(serial_fd, message, length);
    if (ret == -1)
    {
      return errno;
    }
    length -= ret;
    message += ret;
  }
  while (length > 0);

  return 0;
}

int receiveMessage(uint8_t *message, int *length, int timeoutMs)
{
	int headerOffset = 0;
	int inlen = 0;
	int receiveBytesLen =0;
	int retryCount =0;
	int ret = -1;
	int i = 0;
	int len;

	  headerOffset = 0;
	  receiveBytesLen = 7;
retryHeader:
	  inlen = 0;	  
	  *length = 0;
	  retryCount++;
	  ret = s_receiveBytes(receiveBytesLen - headerOffset, &inlen, message + headerOffset, timeoutMs);
	  if (0 != ret)
	  {
		/* @todo Figure out how many bytes were actually obtained in a failed receive */
		return ret;
	  }
	  if (message[0] != (uint8_t)0xFF)
	  {
		for (i = 1; i < receiveBytesLen; i++) 
		{
		  if (message[i] == 0xFF)
		  {
			/* An SOH has been found, so we won't enter the "NO SOH" section of code again */
			headerOffset = receiveBytesLen - i;
			memmove(message, message + i, headerOffset);
			goto retryHeader;
		  }
		}
		if (retryCount < 10)
		{
		  /* Retry to get SOH */
		  goto retryHeader;
		}
		return -1;
	  }
	
	  *length = receiveBytesLen;
	  /* After this point, we have the the bare minimum (5 or 7)  of bytes in the buffer */
	
	  /* Layout of response in data array: 
	   * [0] [1] [2] [3]	  [4]	   [5] [6]	... [LEN+4] [LEN+5] [LEN+6]
	   * FF  LEN OP  STATUSHI STATUSLO xx  xx	... xx		CRCHI	CRCLO
	   */
	  len = message[1];
	
	  if (0 == len)
	  {
		inlen = 0;
	  }
	  else if ((256 - receiveBytesLen) < len)
	  {
		/**
		 * packet data size is overflowing the buffer size. This could be a
		 * corrupted packet. Discard it and move on. Return back with TMR_ERROR_TOO_BIG
		 * error.
		 **/
		return -1;
	  }
	  else
	  {
		ret = s_receiveBytes(len, &inlen, message + receiveBytesLen, timeoutMs);
	  }

	*length = receiveBytesLen + inlen; 
	return 0;
}


int serial_receiveBytes(uint8_t *message, int *len, int timeoutMs)
{
  int ret;
  struct timeval tv;
  fd_set set;
  int status = 0;

  FD_ZERO(&set);
  FD_SET(serial_fd, &set);
  tv.tv_sec = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;
  /* Ideally should reset this timeout value every time through */
  ret = select(serial_fd + 1, &set, NULL, NULL, &tv);
  if (ret < 1)
  {
  	printf("80\n");
    return -1;
  }
  ret = read(serial_fd, message, 256);
  if (ret == -1)
  {
    return -1;
  }

  if (0 == ret)
  {
      /**
       * We should not be here, coming here means the select()
       * is success , but we are not able to read the data.
       * check the serial port connection status.
       **/
    ret = ioctl(serial_fd, TIOCMGET, &status);
    if (-1 == ret)
    {
        /* not success. check for errno */
      if (EIO == errno)
      {
          /**
           * EIO means I/O error, may serial port got disconnected,
           * throw the error.
           **/
          return -1;
      }
    }
  }
  *len = ret;
  return 0;
}

int s_receiveBytes(uint32_t length, uint32_t *messageLength, uint8_t* message, const uint32_t timeoutMs)
{
  int ret;
  struct timeval tv;
  fd_set set;
  int status = 0;

  *messageLength = 0;
  do
  {
    FD_ZERO(&set);
    FD_SET(serial_fd, &set);
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    /* Ideally should reset this timeout value every time through */
    ret = select(serial_fd + 1, &set, NULL, NULL, &tv);
    if (ret < 1)
    {
    	printf("209!\n");
      return -1;
    }
    ret = read(serial_fd, message, length);
    if (ret == -1)
    {
    
	printf("216!\n");
      if (ENXIO == errno)
      {
        return -1; 
      }
      else
      {
        return -1;
      }
    }

    if (0 == ret)
    {
      /**
       * We should not be here, coming here means the select()
       * is success , but we are not able to read the data.
       * check the serial port connection status.
       **/
      ret = ioctl(serial_fd, TIOCMGET, &status);
      if (-1 == ret)
      {
        /* not success. check for errno */
        if (EIO == errno)
        {
          /**
           * EIO means I/O error, may serial port got disconnected,
           * throw the error.
           **/
          return -1;
        }
      }
    }

    length -= ret;
    *messageLength += ret;
    message += ret;
  }
  while (length > 0);

  return 0;
}


int serial_setBaudRate(uint32_t rate)
{
	printf("rate = %d\n",rate);
    struct termios t;

    tcgetattr(serial_fd, &t);

#define BCASE(t,n) case n: cfsetispeed((t),B##n); cfsetospeed((t),B##n); break;
    switch (rate)
    {
      BCASE(&t, 9600);
      BCASE(&t, 19200);
      BCASE(&t, 38400);
      // Believe it or not, speeds beyond 38400 aren't required by POSIX.
#ifdef B57600
      BCASE(&t, 57600);
#endif
#ifdef B115200
      BCASE(&t, 115200);
#endif
#ifdef B230400
      BCASE(&t, 230400);
#endif
#ifdef B460800
      BCASE(&t, 460800);
#endif
#ifdef B921600
      BCASE(&t, 921600);
#endif
    default:
      return -1;
    }
#undef BCASE
    if (tcsetattr(serial_fd, TCSANOW, &t) != 0)
    {
      return errno;
    }

  return 0;
}

int serial_shutdown()
{

  close(serial_fd);
  /* What, exactly, would be the point of checking for an error here? */

  return 0;
}

int serial_flush()
{
  if (tcflush(serial_fd, TCOFLUSH) == -1)
  {
    return errno;
  }

  return 0;
}

