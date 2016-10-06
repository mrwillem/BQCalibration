#ifndef SERIAL_H
#define SERIAL_H

#define _GNU_SOURCE
#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyACM1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

/*
 * possible return strings from microcontroller get integers
 */
#define EI2CDATA 0
#define EI2CINVALID 1

#define EI2CSEND 3
#define RESEND 4
#define FULLRESEND 5
#define EXIT_PROGMODE 6
extern volatile int STOP;

int lock_bq34(int);
int unlock_bq34(int);
int bq34_read(int, uint8_t, uint8_t *,  uint8_t *);
int bq34_write(int, uint8_t*,  uint8_t);
int serial_open( int *, char*);
void serial_close( int *);
int serial_receive(int , uint8_t * );
int serial_set_blocking (int , int );
int serial_set_interface_attribs (int, int, int);

#endif // SERIAL_H
