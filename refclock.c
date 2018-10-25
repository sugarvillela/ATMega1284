#include <stdio.h>
#include "refclock.h"
#include "lcddual.h"// uchar; delete this

#define RFC_PERIOD	10

typedef struct {
	ushort raw;
	uchar min;
	uchar sec;
	uchar frame;
} referenceClock;

referenceClock refClock;

ushort getTime(){
    return refClock.raw;
}
void getTimeMSF( char buf[] ){
    uchar iTo=0, iFrom;
    uchar nBuf[4];
    nToChars( (ushort)refClock.min, nBuf );
    if(refClock.min<10){
        buf[iTo++]='0';
    }
    for( iFrom=0;iFrom<4;iFrom++){
        if( !nBuf[iFrom] ){ break; }
        buf[iTo++]=nBuf[iFrom];
    }
    buf[iTo++]=':';
    nToChars( (ushort)refClock.sec, nBuf );
    if(refClock.sec<10){
        buf[iTo++]='0';
    }
    //printf("sec=%d, nBuf=%s\n", refClock.sec, nBuf );
    for( iFrom=0;iFrom<4;iFrom++){
        if( !nBuf[iFrom] ){ break; }
        buf[iTo++]=nBuf[iFrom];
    }
    buf[iTo++]=':';
    nToChars( (ushort)refClock.frame, nBuf );
    if(refClock.frame<10){
        buf[iTo++]='0';
    }
    //printf("frame=%d, nBuf=%s\n", refClock.frame, nBuf );
    for( iFrom=0;iFrom<4;iFrom++){
        buf[iTo++]=nBuf[iFrom];
        if( !nBuf[iFrom] ){ break; }
    }
}
void refclockInit(){
    printf("refclock init\n");
	refClock.min=0;
	refClock.sec=0;
	refClock.frame=0;//100 frames/sec
	refClock.raw=0;
}
/* State machine tick function */
enum rfc_states { RFC_INIT, RFC_COUNT };//period = 10 ms
int refClock_tick( int state ){
    //printf("refclock init???\n");
    //refclockInit();
	switch(state) {
		case RFC_INIT:
		    refclockInit();
		    state=RFC_COUNT;
		    break;
		case RFC_COUNT:
        	refClock.frame+=RFC_PERIOD;
        	refClock.raw+=RFC_PERIOD;
            if( refClock.frame ==100 ){
                refClock.sec++;
                refClock.frame=0;
            }
            if( refClock.sec==60 ){
                refClock.min++;
                refClock.sec=0;
            }
		    break;
		default:
		    state=RFC_INIT;
			break;
	}

	return state; 
}

void refClockTest(){
	int i, state=-1;
	unsigned char buf[16];
    for( i=1;i<3000;i++){
    	state=refClock_tick( state );
		if( i%100==0){
			getTimeMSF( buf );
			printf("====================\n");
			printf("%d\n", getTime());
			printf("%s\n", buf);
		}
    }
}