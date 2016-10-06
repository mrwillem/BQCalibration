#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include "convert.h"
#include "serial.h"
#include "BQCalib.h"
#include "bqfloat.h"
/*
*for sleep function
*/
#include <time.h>


/*
* A function that sleeps for the given amount of milliseconds 
*/
void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}



int main(int argc, char **argv)
{
	/* file descriptor for the terminal */
	int termfd;
	/* pointer to string to hold one line of the configuration file */
	char * linebuf;
	/* pointer to array that holds the decoded line of the configuration file */
	uint8_t *recbuf;
	/* pointers to array that hold the serial communication data */
	uint8_t *sendbuf;
	uint8_t *registerdata;
	/* variable that holds the length of linebuf, may be changed by readline function */
	size_t len = 1023;
	/* used to store the number of bytes read from file */
	int read;
	int ret;
	/* variable for the sleep function */
	int sleep;
	
	int progresscounter;

	int i;
	/* the variable stores an integer of which calibration mode is selected i.e. voltage or board_offset */
	int subcmd;
	
	/*some variables for checksum calculation and matching */
	int tmpchecksum;
	uint8_t checksum;
	uint8_t current_checksum;

	/* variables that keep track of the current state of the BQxxx device */
	int mode;
	int calen;

	uint16_t *helperpointer;
	int calculate;
	int samplecounter;
	/* some variables for voltage calibration */
	int setvoltage;
	int readvoltage;
	int voltagedifference;
	uint setvoltagedivider;
	uint newvoltagedivider;
	
	/* some variables for current calibration */
	int multicurrent;
	int16_t boardcurrent;  
	int avgcurrent;
	float ccgain;
	float ccdelta;
	float ccgainmohm;
	float ccdeltamohm;
	int16_t cc_offset;
	signed char board_offset;
	uint8_t newreg[4];
	
	/* a integer to store numbers returned by readline */
	int readcharacters;
	mode=0;
	int c;
	
	if(print_welcome(argc))
	{
		return -1;
	}
	else
	{
		if(serial_open(&termfd, argv[1]) == 0)
		{
			if(serial_set_interface_attribs (termfd, BAUDRATE,  0) != 0)
			{
				perror("could not set serial mode");
				return -1;
			}
		}
		else
		{
			perror("could not open serial port");
			return -1;
		}
	}


	/*
	 *Get memory for buffers
	 */
	linebuf=(char *) malloc((len+1)*sizeof(char));
	recbuf=(uint8_t *) malloc(129*sizeof(uint8_t));
	sendbuf=(uint8_t *) malloc(128*sizeof(uint8_t));
	registerdata=(uint8_t *) malloc(128*sizeof(uint8_t));
	if((registerdata == NULL) || (recbuf == NULL) || (linebuf == NULL) || (sendbuf == NULL))
	{	
		printf("Error allocating memory for buffers\n");
		return 1;
	}
	subcmd=get_subcommand(argv[2]);	
	
	/*
	 * This locks the BQChip in the host so no other communication should take place
	 */
	while(mode != 2)
	{
		mode = lock_bq34(termfd);
	}

	switch(subcmd)
	{
		case CMDERROR:
			//cleanup;
			break;
		case CMDVOLTAGE:
			printf("Will calibrate the voltage. \n For this you need a multimeter connected on the + and - side of the battery\n");
			while(1)
			{
				/* read the current settings of the register */
				printf("Will read current voltage divider\n");
                sendbuf[0]=0x3E;sendbuf[1]=0x68;
                bq34_write(termfd, sendbuf, 2);
                sendbuf[0]=0x3F;sendbuf[1]=0x00;
                bq34_write(termfd, sendbuf, 2);
				tmpchecksum=0x00;
				read=32;
				bq34_read(termfd, 0x40, recbuf, &read);
				for(i=0; i<read; i++)
				{
					registerdata[i]=recbuf[i];
					tmpchecksum+=recbuf[i];
				}
				checksum=(uint8_t)(tmpchecksum/256);
				tmpchecksum=tmpchecksum-(checksum*256);
				checksum=0xFF-tmpchecksum;
				read=1;
				bq34_read(termfd, 0x60, recbuf, &read);
				current_checksum=recbuf[0];
				if(current_checksum == checksum)
				{
					setvoltagedivider=((registerdata[14]*256)+registerdata[15]);
					printf("Checksum matches - We retrieved correct block data\n");
					printf("The current voltage divider is set to: %d\n", setvoltagedivider);
				}
				else
				{
					continue;
				}
				printf("Will now read the voltage measured by your device.\nPrepare to enter the value you read on your multimeter in millivolt\n i.e. 12V=12000mV\n");
				read=2;
				while(bq34_read(termfd, 0x08, recbuf, &read));
                readvoltage=recbuf[1]*256+recbuf[0];
				printf("The device measures %d mV\n", readvoltage);
				printf("Please provide measured voltage\nWrite exit to exit calibration\n");
				readcharacters = getline(&linebuf,&len, stdin);
				if(strncmp(linebuf, "exit", 4) == 0)
				{
					printf("You wrote exit\n Will exit now.\n");
					break;
				}
				setvoltage=0;
				for(i=0;i<readcharacters;i++)
				{
					if((linebuf[i] >0x29) && (linebuf[i] <	0x40))
					{
						setvoltage*=10;
						setvoltage+=linebuf[i]-0x30;
					}
				}
				printf("You measured %d mV is that OK?. \nPlease type yes or no\n", setvoltage);
				readcharacters = getline(&linebuf, &len, stdin);
				if(strncmp(linebuf, "yes", 3) == 0)
				{
					newvoltagedivider=setvoltagedivider*setvoltage/readvoltage;
				}
				helperpointer=&registerdata[14];
				*helperpointer=newvoltagedivider;
				tmpchecksum=0x00;
				for(i=0;i<32;i++)
				{
					tmpchecksum+=registerdata[i];
				}

			}

			break;
		case CMDCURRENT:
			printf("Put your multimeter in line with the complete load to measure current accurately\n");
			printf("Are your set up? if yes type yes otherwise type something else to leave the program\n");
			printf("Please observe the current reading for about 15 seconds after you typed yes on your multimeter\n");
			printf("You have to enter an average value of the current that was measured in the time about 15 seconds after you typed yes\n");
			printf("\nNote 1: In scaled mode you have to divide your measured current by the same factor that the other values are scaled by \n");
			printf("Note 2: Current calibration will give no good results if you did not perform board_offset calibration before\n\n");
			printf("If you have read everything and know what to do type yes now and observe the multimeter\n");
			readcharacters = getline(&linebuf, &len, stdin);
			/* leave if user typed something different than yes */
			if(strncmp(linebuf, "yes", 3) != 0)
				break;
			avgcurrent=0;
			samplecounter=0;
			for(i=0; i<31; i++)
			{
				read=2;
				while(bq34_read(termfd, 0x10, recbuf, &read));
				/* Don't know if this is necessary to get the sign... */
				/* board current is int16_t averagecurrent is int */ 
				boardcurrent=((int16_t) recbuf[1]<<8) | recbuf[0];
				if(boardcurrent < 0)
				{
					samplecounter++;
					avgcurrent+=(-1*boardcurrent);
					avgcurrent = i >0 ? (avgcurrent/2) : avgcurrent;
				}
				/* wait 500 ms for next measurement */
				printf(".");
				sleep_ms(500);
			}
			printf("Measured %d samples - The average current was %d mA\n", samplecounter, avgcurrent);
			printf("\n\nPlease enter now the average current you read during the last 15 seconds as seen on the multimeter\n");
			printf("or quit if you want to cancel\n");
			readcharacters = getline(&linebuf, &len, stdin);
			if(strncmp(linebuf, "quit", 4) == 0)
				break;
			multicurrent=0;
			for(i=0;i<readcharacters;i++)
			{	
				if((linebuf[i]>0x29) && (linebuf[i] <0x40))
				{
					multicurrent*=10;
					multicurrent+=linebuf[i]-0x30;
				}
			}
			/* read the current settings of the register */

			printf("Will read current cc_offset and board_offset \n");
            sendbuf[0]=0x3E;sendbuf[1]=0x68;
            bq34_write(termfd, sendbuf, 2);
            sendbuf[0]=0x3F;sendbuf[1]=0x00;
            bq34_write(termfd, sendbuf, 2);
			tmpchecksum=0x00;
			read=0x32;
			bq34_read(termfd, 0x40, recbuf, &read);
			for(i=0; i<read; i++)
			{
				registerdata[i]=recbuf[i];
				tmpchecksum+=recbuf[i];
			}
			checksum=(uint8_t)(tmpchecksum/256);
			tmpchecksum=tmpchecksum-(checksum*256);
			checksum=0xFF-tmpchecksum;
			sleep_ms(100);
			read=1;
			bq34_read(termfd, 0x60, recbuf, &read);
			current_checksum=recbuf[0];
			if(current_checksum == checksum)
			{
				cc_offset=(((uint16_t)registerdata[8]<<8)+(uint16_t)registerdata[9]);
				board_offset=registerdata[10];
				ccgain=(float)(multicurrent/(float)((int)avgcurrent-((cc_offset+board_offset)/16)));
				ccdelta=ccgain*1193046;
				printf("ccgain=%3.5f\n", ccgain);
				ccgainmohm=4,768/ccgain;
				printf("CC Gain=%3.5f mOhm\n", ccgainmohm);
				floating2Byte(ccgain, newreg);
				registerdata[0]=newreg[0];
				registerdata[1]=newreg[1];
				registerdata[2]=newreg[2];
				registerdata[3]=newreg[3];
				printf("ccdelta=%3.5f\n", ccdelta);
				ccdeltamohm=5677445/ccdelta;
				printf("CC Delta=%3.5f mOhm\n", ccdeltamohm);
				floating2Byte(ccdelta, newreg);
				registerdata[4]=newreg[0];
				registerdata[5]=newreg[1];
				registerdata[6]=newreg[2];
				registerdata[7]=newreg[3];
				for(i=0; i<32; i++)
				{
					registerdata[i]=recbuf[i];
					tmpchecksum+=recbuf[i];
				}
				checksum=(uint8_t)(tmpchecksum/256);
				tmpchecksum=tmpchecksum-(checksum*256);
				checksum=0xFF-tmpchecksum;
				printf("New_checksum is %d\n", checksum);
				printf("Program Values? Type yes if values should be programmed\n");
				readcharacters = getline(&linebuf, &len, stdin);
				if(strncmp(linebuf, "yes", 3) != 0)
					break;
				sendbuf[0]=0x40;
				sendbuf[1]=registerdata[0];sendbuf[2]=registerdata[1];sendbuf[3]=registerdata[2];
				sendbuf[4]=registerdata[3];sendbuf[5]=registerdata[4];sendbuf[6]=registerdata[5];
				sendbuf[7]=registerdata[6];sendbuf[8]=registerdata[7];
				// bq34_write(termfd, sendbuf, 9);
				sleep_ms(100);
				sendbuf[0]=0x60;
				sendbuf[1]=checksum;
				// bq34_write(termfd, sendbuf, 2);
				sleep_ms(1000);

			}


					
			break;
		case CMDCCOFFSET:
			/* Enable calibration mode */
			calen=1;
			while(calen)
			{
				sendbuf[0]=0x00;sendbuf[1]=0x2D;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				sendbuf[0]=0x00;sendbuf[1]=81;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				sendbuf[0]=0x00;sendbuf[1]=0x00;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf,3);
				sleep_ms(100);
				read=2;
				while(bq34_read(termfd, 0x00, recbuf, &read));
				if((recbuf[1] & 0b00010000) == 0b00010000)
					calen=0;
			}

			break;
		case CMDTEMPERATURE:
			break;
		case CMDBOARDOFFSET:
			printf("Enabeling calibration mode\n");
			/* enable calibration mode */
			calen=1;
			while(calen)
			{
				sendbuf[0]=0x00;sendbuf[1]=0x2D;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				sendbuf[0]=0x00;sendbuf[1]=0x81;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				sendbuf[0]=0x00;sendbuf[1]=0x00;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				read=2;
				while(bq34_read(termfd, 0x00, recbuf, &read));
				if((recbuf[1] & 0b00010000) == 0b00010000)
					calen=0;
			}
			printf("Calibration mode enabled now sending calibrate board offset\n");
			printf("On some devices this also calibrates cc offset...\n");
			printf("Take a look at your datasheet\n");
			calen=3;
			while(calen==3)
			{
				sendbuf[0]=0x00;sendbuf[1]=0x09;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sendbuf[0]=0x00;sendbuf[1]=0x00;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				read=2;
				while(bq34_read(termfd, 0x00, recbuf, &read));
				/* 0b00001100 means cca and bca are enabled */
				if((recbuf[1] & 0b00001100) == 0b00001100)
					calen=0;

			}
			calen=2;
			progresscounter=0;
			printf("Calibration started waiting for calibration to finish\n");
			printf("This may take some time\n");
			while(calen==2)
			{
				sendbuf[0]=0x00;sendbuf[1]=0x00;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(500);
				read=2;
				while(bq34_read(termfd, 0x00, recbuf, &read));
				/* wait for bca and cca bit to clear*/
				if((recbuf[1] & 0b00001100) == 0b00000000)
					calen=0;
				if(progresscounter < 50)
					{printf(".");progresscounter++;}
				else
					{printf(".\n");progresscounter=0;}

			}
			printf("\nCalibration finished\n Will now save calibration values\n");
			sendbuf[0]=0x00;sendbuf[1]=0x0B;sendbuf[2]=0x00;
			bq34_write(termfd, sendbuf, 3);
			sleep_ms(100);
			calen=1;
			while(calen==1)
			{
				sendbuf[0]=0x00;sendbuf[1]=0x80;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(10);
				sendbuf[0]=0x00;sendbuf[1]=0x00;sendbuf[2]=0x00;
				bq34_write(termfd, sendbuf, 3);
				sleep_ms(100);
				read=2;
				while(bq34_read(termfd, 0x00, recbuf, &read));
				/* wait for bca and cca bit to clear*/
				if((recbuf[1] & 0b00010000) == 0b00000000)
					calen=0;

			}
			printf("Board Calibration complete. Now you can calibrate voltage, current and temperature\n");
			break;
		default:
			// cleanup
			break;

	}

	while(mode != 1)
	{
		mode = unlock_bq34(termfd);
	}
	if(linebuf)
       	free(linebuf);
	if(recbuf)
		free(recbuf);
	if(sendbuf)
		free(sendbuf);
	if(registerdata)
		free(registerdata);
	exit(0);
}

int get_subcommand(char *command)
{
	if(strlen(command) < 7)
		return CMDERROR;
	if(strncmp(command, "voltage", 7) == 0)
		return CMDVOLTAGE;
	else if(strncmp(command, "current", 7) == 0)
		return CMDCURRENT;
	else if(strncmp(command, "cc_offset", 9)==0)
		return CMDCCOFFSET;
	else if(strncmp(command, "temperature", 10)==0)
		return CMDTEMPERATURE;
	else if(strncmp(command, "board_offset", 12) == 0)
		return CMDBOARDOFFSET;
	else
		return CMDERROR;
}

int print_welcome(int argument_count)
{
	printf("Welcome to BQ34Calib, a little shell script to calibrate TIs BQ Series devices\n");
	printf("This program needs some sort of a USB serial - i2c gateway\n");
	printf("You need the serial commands \"bqx_send char data[]\" and \"bqx_read register number\"  implemented\n\n");
	printf("The tool accepts the commands cc_offset, board_offset, voltage, current and temperature\n");
	printf("Calibration should take place in the order above 1. cc_offset then board_offset and so on\n");
	printf("Some commands will require that you enter measured values while calibrating\n");
	printf("Note when using scaled mode the CURRENT while CALIBRATING has to be SCALED by the same FACTOR\n");
	if(argument_count != 3)
	{
		printf("\n\nYou provided %d arguments\nPlease provide the serial port and the command i.e. BQCalib /dev/ttyACM0 cc_offset\n", argument_count);
		return 1;   
	}
	return 0;
}
