#ifndef REFCLOCK_H_
#define REFCLOCK_H_

#define REFCLK_PERIOD 50 //10 ms

void refclockOn();
void refclockOff();
void zeroRefclock();
unsigned char refclockMark();

unsigned short getTime();
void getTimeMSF( char buf[] );
int refclock_tick( int state );

#endif /* REFCLOCK_H_ */