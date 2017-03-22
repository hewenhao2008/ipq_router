#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#include "common_def.h"
#include "serial_dev.h"

#define SERIAL_PORT "/dev/ttyUSB0"

struct serial_dev gSerialDev;


// 115200 8n1 
int zb_set_serial_option(int fd, int nSpeed, int nBits, char nParity, int nStop)
{
	struct termios oldtio, newtio;

	if( tcgetattr(fd, &oldtio) != 0)
	{
		ZB_ERROR("tcgetattr failed.\n");
		return -1;
	}

	memset(&newtio, 0, sizeof(newtio));
	//newtio.c_cflag |= CLOCAL | CREAD; 
	//newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag &= ~(ECHO | CLOCAL | CREAD);

	//enable hw flow control. RTS/CTS
	//newtio.c_cflag |= CRTSCTS;
	
	if( nBits == 8)
	{
		newtio.c_cflag |= CS8;	//nbits = 8
	}
	else 
	{
		newtio.c_cflag |= CS7;	//nbits = 8
	}

	switch( nParity)
	{
		case 'O':	//奇校验
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E':     //偶校验   	
			newtio.c_iflag |= (INPCK | ISTRIP);
	        newtio.c_cflag |= PARENB;
	        newtio.c_cflag &= ~PARODD;
	        break;
	    case 'n':	//无
	    	newtio.c_cflag &= ~PARENB;
        	break;	        
	}

	switch(nSpeed)
	{
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		default:
			 cfsetispeed(&newtio, B9600);
        	 cfsetospeed(&newtio, B9600);
			break;
	}

	switch( nStop)
	{
		case 1:
			newtio.c_cflag &=  ~CSTOPB;
			break;
		case 2:
			newtio.c_cflag |=  CSTOPB;
			break;
    	default:
    		newtio.c_cflag &=  ~CSTOPB;
			break;    		
	}

	newtio.c_cc[VTIME]  = 100;	//wait 10 seconds if no data available.
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    
    if((tcsetattr(fd, TCSANOW, &newtio))!=0)
    {
        ZB_ERROR("Failed to set serial option. tcsetattr.\n");
        return -1;
    }

   	return 0;
}

int zb_open_serial()
{
	int fd;
	//int flags;
	int ret;


	//fd = open(SERIAL_PORT, O_RDWR|O_NOCTTY|O_NDELAY);	//no blocking.
	fd = open(SERIAL_PORT, O_RDWR|O_NOCTTY);
	if(-1 == fd)
	{
		ZB_ERROR("Failed to open serial port.[%s]\n", SERIAL_PORT);
		return -1;
	}

	//set serial non-blocking.
	//flags = fcntl(fd, F_GETFL, 0);
	//flags |= O_NONBLOCK;
	//fcntl(fd, F_SETFL, flgas);

	ret = zb_set_serial_option(fd, 115200, 8, 'n', 1);
	if( ret < 0)
	{
		ZB_ERROR("zb_set_serial_option Failed.\n");
		return -1;
	}

	return fd;	
}

int zb_send(zb_uint8 *buf, zb_uint16 bufLen)
{
	ssize_t n;
	
	n = write(gSerialDev.fd, buf, bufLen);
	if( n < bufLen)
	{
		return -1;	//Failed to send.
	}

	return 0;
}


int zb_init_serial(void)
{
	memset(&gSerialDev, 0, sizeof(struct serial_dev));

	gSerialDev.fd = zb_open_serial();
	if( gSerialDev.fd < 0 )
	{
		return -1;
	}

	return 0;	
}

int zb_serial_read_blocking(zb_uint8 *buf, int len)
{
	int nr;
	
	nr = read(gSerialDev.fd, buf, len);	//blocking read.
	if( nr > 0)
	{
		return nr;
	}
	else
	{
		ZB_DBG(">>>	dbg, zb_read_blocking timeout, No data. nr=%d\n", nr);
	}
	return nr;	
}

void zb_serial_read_loop(int fd, serail_data_in_cb f_serial_data_in, int timeout_ms)
{

	int ret;
	int nr;
	struct timeval tv;
	
#define SERIAL_BUF_SIZE 256

	char buf[SERIAL_BUF_SIZE];

	while(1)
	{
		FD_ZERO(&gSerialDev.rd_set);
		
		FD_SET(gSerialDev.fd, &gSerialDev.rd_set);
		gSerialDev.fd_max = gSerialDev.fd + 1;	

		tv.tv_sec = timeout_ms/1000;
		tv.tv_usec = timeout_ms*1000 - tv.tv_sec*1000*1000;
		
		ret = select( gSerialDev.fd_max , &gSerialDev.rd_set, NULL, NULL, &tv);
		if( ret < 0)
		{
			//error .
		}
		else if( ret == 0)
		{
			//timeout
		}

		if( gSerialDev.stop)
		{
			ZB_PRINT("--- stop read data from serial.\n");
			return;
		}

		if( FD_ISSET(fd, &gSerialDev.rd_set))	//has data.
		{
			//has data to read.
			nr = read(fd, buf, SERIAL_BUF_SIZE);
			if( nr > 0)
			{
				f_serial_data_in(buf, nr);
			}			
		}
	}	

}

