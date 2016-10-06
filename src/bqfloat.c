#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

void floating2Byte(float value, uint8_t * rawData)
{
	int i;
	float CC_value;
	//Read CC_gain or CC_delta value from the gauge.
	int exp;
	//Initialize the exponential for the floating to byte conversion 
	float val;
	float mod_val;
	float tmpVal;
	unsigned char byte0, byte1, byte2;

	CC_value=value;
	val = CC_value;
	exp=0;
	if(val < 0)
	{
		mod_val	= val * -1;
	}
	else
	{
		mod_val = val;
	}
	tmpVal = mod_val;
	tmpVal = tmpVal * (1 + pow(2, -25));
	if(tmpVal < 0.5)
	{
		while (tmpVal < 0.5)
		{
			tmpVal*=2;
			exp--;
		}
	}
	else if (tmpVal <= 1.0)
	{
		while(tmpVal >= 1.0)
		{
			tmpVal=tmpVal/2;
			exp++;
		}
	}	
	if(exp>127)
	{
		exp=127;
	}
	else if(exp < -128)
	{
		exp=-128;
	}
	tmpVal=pow(2,8-exp)*mod_val-128;
	byte2 =floor(tmpVal);
	tmpVal = pow(2, 8) * (tmpVal - (float) byte2);
	byte1=floor(tmpVal);
	tmpVal=pow(2,8)*(float)(tmpVal-(float)byte1);
	byte0=floor(tmpVal);
	if(val<0)
	{
		byte2=(byte2|0x80);
	}
	rawData[0]=exp+128;
	rawData[1]=byte2;
	rawData[2]=byte1;
	rawData[3]=byte0;
	for(i=0;i<4;i++)
	{
		printf("rawData[%d] is %x\n", i, rawData[i] & 0xff);
	}
}
