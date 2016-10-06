#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "serial.h"
#include "convert.h"
#include "BQCalib.h"





volatile int STOP = 0;

int bq34_write(int termfd, uint8_t *data,  uint8_t length)
{
	int i;
	int k;
	int res;
	uint16_t *helperpointer;
	char buf[256];
	strncpy(buf, "i2c_send ", 9);
	for(i=0;i<length;i++)
	{
		helperpointer=&buf[((i*3)+9)];
		*helperpointer=htoa(data[i]);
		buf[((i*3)+11)]=0x20;
	}
	buf[((length*3)+9)]=0x0d;
	buf[((length*3)+10)]=0x0a;
	write(termfd, buf, ((length*3)+11));
	memset(buf,0,128);
	while (STOP==FALSE)
	{
		/* loop for input */
		res = read(termfd,buf,255);
		buf[res]=0;
		printf(":%s:%d\n", buf, res);
		if (strncmp(buf, "Invalid", 7) == 0)
		{
			return EI2CINVALID;
			STOP=TRUE;
		}
		else if(strncmp(buf, "Data Written", 12) == 0)
		{
			return EI2CSEND;
		}
	}
	return EI2CINVALID;
}



int bq34_read(int termfd, uint8_t bq34_register, uint8_t *recbuf, uint8_t *length)
{

	STOP=FALSE;
	int res;
	int i;
	int k;
	int watchdog;
	char rxbuf[256];
	char buf[256];
	uint16_t *helper_pointer;
	strncpy(buf, "i2c_read ",9);
	helper_pointer=&buf[9];
	*helper_pointer=htoa(bq34_register);
	buf[11]=0x20;
	helper_pointer=&buf[12];
	*helper_pointer=htoa(*length);
	buf[14]=0x0d;
	buf[15]=0x0a;
	buf[16]=0;
	printf("Writing: %s\n", buf);
	write(termfd, buf, 16);
	memset(buf,0,128);
	k=0;
	watchdog=0;
	while (STOP==FALSE)
	{
		/* loop for input */
		watchdog++;
		res = read(termfd,rxbuf,255);
		for(i=0; i<res; i++)
		{
			buf[k]=rxbuf[i];
			k++;
			if(k>128)
			{
				STOP=TRUE;
			}
		}
		if (strncmp(buf, "Invalid", 7) == 0)
		{
			return EI2CINVALID;
			STOP=TRUE;
		}
		else
		{
			for(i=0; i<255; i++)
			{
				if(((buf[i-1]==0x0a) || (buf[i-1]==0x0d)) && (buf[i]== 0x0a))
				{
						*length = atoh(recbuf,buf,i);
					return EI2CDATA;
				}
			}
			if(i==255)
			{
				return EI2CINVALID;
			}
		}
		if(watchdog == 5000)
			return EI2CINVALID;
		sleep_ms(10);
	}
	return EI2CDATA;
}


int lock_bq34(int termfd)
{
	char * buf;
	int res;
	int errorcount;
	errorcount=0;
	buf = (char *) malloc(20*sizeof(char));
	buf=strncpy(buf, "i2c_lock", 8);
	buf[8]=0x0d;
	buf[9]=0x0a;
	printf(" Writing %s to %i\n", buf, termfd);
	write(termfd, buf, 10);
	memset(buf,0,20);
	while (STOP==FALSE)
	{
		/* loop for input */
		res = read(termfd,buf,255);   /* returns after 10 chars have been input */
		buf[res]=0;               /* so we can printf... */
		printf(":%s:%d\n", buf, res);

		if (strncmp(buf, "SetMode", 7) == 0)
		{
			free(buf);
			return 2;
			STOP=TRUE;
		}
		else if((strncmp(buf, "Invalid", 7) == 0) || (strncmp(buf, "reset_bqtx", 10) == 0))
		{
			if(errorcount < 10)
			{
				errorcount++;
				buf=strncpy(buf, "i2c_lock", 8);
				buf[8]=0x0d;
				buf[9]=0x0a;
				write(termfd, buf, 10);
				memset(buf,0,20);
			}
			else
			{
				free(buf);
				return 1;
				STOP=TRUE;
			}
		}
	}
	free(buf);
	return 1;
}

int unlock_bq34(int termfd)
{
	char * buf;
	int res;
	int errorcount;
	errorcount=0;
	buf = (char *) malloc(20*sizeof(char));
	buf=strncpy(buf, "i2c_free", 8);
	buf[8]=0x0d;
	buf[9]=0x0a;
	printf(" Writing %s to %i\n", buf, termfd);
	write(termfd, buf, 10);
	memset(buf,0,20);
	while (STOP==FALSE)
	{
		/* loop for input */
		res = read(termfd,buf,255);   /* returns after 10 chars have been input */
		buf[res]=0;               /* so we can printf... */
		printf(":%s:%d\n", buf, res);

		if (strncmp(buf, "SetMode", 7) == 0)
		{
			free(buf);
			return 1;
			STOP=TRUE;
		}
		else if((strncmp(buf, "Invalid", 7) == 0) || (strncmp(buf, "reset_bqtx", 10) == 0))
		{
			if(errorcount < 10)
			{
				errorcount++;
				buf=strncpy(buf, "i2c_free", 8);
				buf[8]=0x0d;
				buf[9]=0x0a;
				write(termfd, buf, 10);
				memset(buf,0,20);
			}
			else
			{
				free(buf);
				return 0;
				STOP=TRUE;
			}
		}
	}
	free(buf);
	return 0;
}

int serial_open( int *termfd, char *device)
{
	*termfd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
	if (*termfd <0)
	{
		perror(device);
		return -1;
	}
	return 0;
}

void serial_close( int *termfd)
{
	// tcsetattr(termfp,TCSANOW,&oldtio);
	close(*termfd);
}


int serial_set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                perror("serial_set_interface_attribs");
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                perror("serial_set_interface_attribs");
                return -1;
        }
        return 0;
}

int serial_set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                perror("tcgetattr");
                return -1;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
                perror("tcsetattr");
		return -1;
	}
	return 0;
}
