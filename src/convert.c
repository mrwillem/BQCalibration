#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdint.h>
#include "convert.h"

#define uchar unsigned char
/*
* A function to get hex bytes from ascii
*/
int atoh(uint8_t * recbuf,char * buf, int len)
{
	int i;
	int k;
	uint8_t checksum;
	k=0;
	for(i=0; i<len; i++)
	{
		/*
		 * if value smaller 0x3A it is a digit
		 */
		if(buf[i]> 0x29 && buf[i] < 0x3A)
		{
			recbuf[k]=(uint8_t)((buf[i]-0x30)<<4);
		}

		/*
		 * if value smaller 0x47 should be an character
		 */
		else if(buf[i]>0x40 && buf[i]< 0x47)
		{
			recbuf[k]=(uint8_t)((buf[i]-0x37)<<4);
		}
		else
		/*
	 	*  Data not valid
	 	*/
		{
			continue;
		}
		/*
	 	* decode the lower nibble
	 	*/
		if(buf[i+1]> 0x29 && buf[i+1] < 0x3A)
		{
			recbuf[k]+=(uint8_t)(buf[i+1]-0x30);
			k++;
			i++;
		}
		else if(buf[i+1]>0x40 && buf[i+1]< 0x47)
		{
			recbuf[k]+=(uint8_t)(buf[i+1]-0x37);
			k++;
			i++;
		}
		else
		{
			recbuf[k]=(uint8_t)0;
		}
	}
	printf("Decoded %i Bytes\n", k);
	return k;
}

/* the function returns the ascii codes for both nibbles of the value provide
 * in one 16 bit integer
 */
uint16_t htoa(uint8_t hex)
{
	uint8_t tmp;
	uint16_t out;
	tmp=(hex & 0b00001111);
	if(tmp > 9)
	{
		out=((tmp+0x37) << 8);
	}
	else
	{
		out=((tmp+0x30) << 8);
	}
	tmp=((hex >> 4) & 0b00001111);
	if(tmp > 9)
	{
		out+=(tmp+0x37);
	}
	else
	{
		out+=(tmp+0x30);
	}
	return out;
}
