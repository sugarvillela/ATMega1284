//#include <stdio.h>
#include <avr/io.h>
#include "refclock.h"
#include "lcddual.h"
#include "UT.h"

enum rfc_states { OFF, ON };//period = 10 ms
	
typedef struct {
	uchar enable;	//on off
	ushort raw;		//total 100th's of a second since start
	uchar min;
	uchar sec;
	uchar frame;	//10th's of a second since second
	int state;
} referenceClock;
referenceClock refclock;

void refclockInit(){
	refclock.min=0;
	refclock.sec=0;
	refclock.frame=0;//100 frames/sec
	refclock.raw=0;
	refclock.state=OFF;
}
void zeroRefclock(){
	refclockInit();
}
void refclockOn(){
	refclock.state=ON;
}
void refclockOff(){
	refclock.state=OFF;
}
unsigned char refclockMark(){//true on second mark
	return refclock.frame<10;
}

int refclock_tick( int state ){
	switch(refclock.state) {
		case OFF:
			return 0;
		case ON:
			refclock.frame++;
			refclock.raw++;
			if( refclock.frame==100 ){//increment seconds
				refclock.sec++;
				refclock.frame=0;
				if( refclock.sec==60 ){//increment minutes
					refclock.min++;
					refclock.sec=0;
				}
			}
	}
	return 0;
}

ushort getTime(){
	return refclock.raw;
}
void getTimeMSF( char buf[] ){
	uchar iTo=0, iFrom;
	uchar nBuf[4];
	nToChars( (ushort)refclock.min, nBuf );
	if(refclock.min<10){
		buf[iTo++]='0';
	}
	for( iFrom=0;iFrom<4;iFrom++){
		if( !nBuf[iFrom] ){ break; }
		buf[iTo++]=nBuf[iFrom];
	}
	buf[iTo++]=':';
	nToChars( (ushort)refclock.sec, nBuf );
	if(refclock.sec<10){
		buf[iTo++]='0';
	}
	for( iFrom=0;iFrom<4;iFrom++){
		if( !nBuf[iFrom] ){ break; }
		buf[iTo++]=nBuf[iFrom];
	}
	buf[iTo++]=':';
	nToChars( (ushort)refclock.frame, nBuf );
	if(refclock.frame<10){
		buf[iTo++]='0';
	}
	for( iFrom=0;iFrom<4;iFrom++){
		buf[iTo++]=nBuf[iFrom];
		if( !nBuf[iFrom] ){ break; }
	}
}
