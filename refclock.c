/****************************************************************************
    This file is part of "Midi Record/Play/Overdub With 5-Pin Connections", 
	"MRecord" for short, Copyright 2018, Dave S. Swanson.

    MRecord is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRecord is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRecord.  If not, see <https://www.gnu.org/licenses/>.
*****************************************************************************/

/*	Refclock:  a tool for timekeeping, needed to set timestamps on new midi notes.  
	Supplies timestamp data as 1/100/sec increments
	Counts seconds and minutes.
	Provides display time as string
*/
#include <avr/io.h>
#include "refclock.h"
#include "lcddual.h"
#include "midiin.h"
#include "UT.h"

enum rfc_states { OFF=0, ON=1 };//period = 10 ms
	
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
void refclockOverride( unsigned short setRaw ){
	/* For transport controls: RW FF */
	refclock.raw=setRaw;
	refclock.frame=setRaw%100;
	refclock.sec=setRaw%60;
	refclock.min=setRaw%3600;
	/* Copy new time to midi currTime */
	midiClockOverride( setRaw );
}

void refclock_tick(){
	if(!refclock.state){ return; }
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
	midi_tick( refclock.raw );//sets midi.currTime for time-stamping
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
