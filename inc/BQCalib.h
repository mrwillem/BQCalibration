#ifndef BQCALIB_H
#define BQCALIB_H
#define CMDERROR 1
#define CMDVOLTAGE 2
#define CMDCURRENT 3
#define CMDCCOFFSET 4
#define CMDTEMPERATURE 5
#define CMDBOARDOFFSET 6

int get_subcommand(char *);

int print_welcome(int);
void sleep_ms(int);

#endif //BQCALIB_H
