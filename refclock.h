#ifndef REFCLOCK_H
#define REFCLOCK_H

unsigned short getTime();
void getTimeMSF( char buf[] );
int refClock_tick( int state );

void refClockTest();

#endif