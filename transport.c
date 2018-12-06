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

#include <avr/io.h>
#include "transport.h"
#include "refclock.h"
#include "lcddual.h"
#include "buttonbus.h"
#include "menu.h"			//for binding stop press event to menu
#include "UT.h"
#include "midiin.h"

/*	Transport syncs with refclock.h and midiin.h. and with beep_tick
	It functions as:
	1.	a metronome: runs beep_tick state; counts and displays M:B time
	2.	interface for controlling clock and midi
*/

#define MAX_BEATS   8       //related to time signatures
#define BEEP_DUR    40		//length of beeps (milliseconds) 
							//Set much lower than least expected beat duration
#define TPS			1000L   //number of ticks per second (clock speed, period)

#define SKIP		10      //Rewind and FF amount
#define OUTPORT PORTB								

typedef struct {
	/* For time-to-beat mapping */
	float b[MAX_BEATS];
	uchar tempo;                //beats per minute
	uchar tsig1;                //number of beats per bar
	uchar tsig2;                //bar length
	uchar currMeasure;          //for display
	uchar currBeat;
	
	/* Display and metronome sound */
	char flasher[5];			//Flash PLAY or REC on LCD
	uchar beepMaskPlay;			//0x18 for light and beep, 0x10 for light only
	uchar beepMaskRec;
	uchar beepMask;	
	
	void (*dispTime)();			//Show M:B or raw time on LCD		
	int state;
	int beepState;
	uchar enable;				//for disabling without disturbing the states
	//uchar recording;
} metronome;
metronome met;

/* Not using PWM; this implements high and low metronome sound */
uchar beepCount;
enum beepStates{ BEEPOFF, HION, HIOFF, LOON1, LOON2, LOOFF1, LOOFF2 };
void beep_tick(){
	switch(met.beepState){
		case BEEPOFF:
			return;
		/* High Cycle */
		case HION:
			OUTPORT |= met.beepMask;
			met.beepState=HIOFF;
			break;
		case HIOFF:
			OUTPORT &=(uchar)~met.beepMask;
			met.beepState=HION;
			break;
		/* Low Cycle */
		case LOON1:
			OUTPORT |= met.beepMask;
			met.beepState=LOON2;
			break;
		case LOON2:
			met.beepState=LOOFF1;
			break;
		case LOOFF1:
			OUTPORT &=(uchar)~met.beepMask;
			met.beepState=LOOFF2;
			break;
		case LOOFF2:
			met.beepState=LOON1;
			break;
	}
	if( ++beepCount>BEEP_DUR ){ 
		CLR_TWO( OUTPORT, 3 );
		met.beepState=BEEPOFF; 
	}
}
void beepHi(){
	met.beepState=HION;
	beepCount=0;
}
void beepLo(){
	met.beepState=LOON1;
	beepCount=0;
}

/* Display Functions */

/* Choice of wrappers for LCD1 display; pointer set by menu */
static char dispBuf[10];
void dispMS(){
	getTimeMSF( dispBuf );//Minutes: Seconds
	LCD_Disp( 17, (const uchar* )dispBuf, 1 );
}
void dispRaw(){
	nToChars( getTime(), (uchar*)dispBuf );//raw
	LCD_Disp( 17, (const uchar* )dispBuf, 1 );
}

/* this updates on each metronome beat */
void dispMetStatus(){
	/* Left Screen */
	static uchar last=1;
	static uchar curr;
	curr=((met.currBeat-1)<<2)+1;
	LCD_Disp( last, (const uchar*)".", 0 );
	LCD_Disp( curr, (const uchar*)"|", 0 );
	last=curr;
	LCD_Cursor( 1, 1 );
	
	/* Right Screen */
	static uchar flash=1;
	getTimeMB( (uchar*)dispBuf );//Measures: Beats
	LCD_Disp( 1, (const uchar* )dispBuf, 1 );
	
	if( flash ){//REC or PLAY
		LCD_Disp( 9, (const uchar* )met.flasher, 1 );
		flash=0;
	}
	else{
		LCD_Disp( 9, (const uchar* )"    ", 1 );
		flash=1;
	}
	met.dispTime();
}

/* this is triggered after welcome screen finishes */
void startupDisplay(){
	msgUnbindDropEvent( 0 );
	//zeroRefclock();
	//met.currMeasure=1;
	//met.currBeat=1;
	//cp( met.flasher, "Stop" );
	transportOn();
}
void initLeftScreen(){
	/*	To save tick time, dispMetStatus() only writes a '|' and a '.'
		The row of dots is written here on startup */
	uchar i;
	for( i=0; i<met.tsig1; i++ ){
		LCD_Disp( (i<<2)+1, (const uchar*)"....", 0 );
	}
}

/* State machine */
enum metStates{ START, ACCENT, BEAT, COUNT };
void metronome_tick(){
	static uchar i=0;					//index of met.b for beat times
	static float count=0;				//for counting up to beat times
	if(!met.enable){ return; }
	/* Two ways of starting: from top of measure or resume in measure 
		set START for the first case, otherwise leave state alone */
	switch (met.state){
		case START:
			i=0;
			count=0;
			met.state=ACCENT;
			break;
		case ACCENT:
			met.state=COUNT;
			break;
		case BEAT:
			met.state=COUNT;
			break;
		case COUNT:
			if( (count+5)>met.b[i] ){
				i++;
				if( i>=met.tsig1 ){
					met.state=ACCENT;
					met.currMeasure++;
				}
				else{
					met.state=BEAT;
				}
			}
			break;
		default:
			met.state=START;
			break;
	}
	switch (met.state){
		case START:
		case COUNT:
			break;
		case ACCENT:
			/* Update measures:beats for display */
			met.currBeat=1;
			/* Turn on high beep */
			beepHi();
			/* Display metronome time */
			dispMetStatus();
			/*	Reset local vars for next measure:
				Notice that it is not set to 0. Since there is
				overrun in detecting the transition point, it seems
				better to keep the difference, to approach the next
				point more accurately. In testing, this method 
				decreased jitter
			 */
			count-=met.b[i-1];
			i=0;
			break;
		case BEAT:
			/* Update measures:beats for display */
			met.currBeat++;
			/* Turn on low beep and set beep duration counter */
			beepLo();
			/* Display metronome time */
			dispMetStatus();
			break;
		default:
			break;
	}
	count+=10;
}

/* Mutators */
void transportInit(){
	/* set defaults */
	met.beepMaskPlay=0x10;
	met.beepMaskRec=0x18;
	met.tempo=120;
	setTimeSignature( FOUR_FOUR );
	met.dispTime=&dispMS; 
	/* Set up transport */
	met.enable=0;
	met.beepState=BEEPOFF;
	/* Zero transport */
	met.state=START;
	zeroRefclock();
	met.currMeasure=1;
	met.currBeat=1;
	cp( met.flasher, "Stop" );
}

void transportOn(){
	msgUnbindDropEvent( 0 );
	bBusInit();
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
    bindPressEvent( 0, &transportStop );// tap to stop
	bindHoldEvent( 0, &menuOn );		// hold to open menu
    bindPressEvent( 2, &transportRW );	// tap to jump back several measures per #define SKIP
    bindHoldEvent( 2, &transportTop );	// hold to jump back to top
	bindPressEvent( 1, &record );		// tap to record
    bindPressEvent( 3, &play );			// tap to play
	bindDoubleEvent( 3, &record );		// also double tap to record
	/* Left Screen needs to be initialized with dots;
		Right screen init to curr measure and time */
	initLeftScreen();
	dispMetStatus();	
}
void play(){
	met.beepMask=met.beepMaskPlay;
	cp( met.flasher, "Play" );
	//if( met.state==OFF ){//preserve the current beat count for resuming
		//met.state=ON;
	//}
	met.enable=1;
	refclockOn();
	midiPlay();
}
void record(){
	msgBindDropEvent( 0, &initLeftScreen );
	msgWriteErr( 0, "Record", MSG_SHORT );
	met.beepMask=met.beepMaskRec;
	cp( met.flasher, "Rec " );
	bReset( 3 );//clear button count so it doesn't call on every subsequent click
	//if( met.state==OFF ){
		//met.state=ON;
	//}
	met.enable=1;
	refclockOn();
	midiRec();
}
void transportStop(){
	//met.state=OFF;
	met.enable=0;
	refclockOff();
	midiOff();
	cp( met.flasher, "Stop" );
	OUTPORT &=0xE7;				//kill the beep port
	dispMetStatus();
	bReset( 3 );
}
void transportTop(){
	bReset( 0 );//clear count so it doesn't call on every subsequent click
	met.state=START;
	zeroRefclock();
	midiTop();
	met.currMeasure=1;
	met.currBeat=1;
	cp( met.flasher, "Stop" );
	LCD_ClearScreen( 1 );
	dispMetStatus();
}
void transportRW(){
	transportStop();
	uchar prev=met.currMeasure;
	met.currMeasure-=SKIP;
	if( met.currMeasure==0 || met.currMeasure > 0xFF-SKIP ){
		transportTop();
	}
	else{//( ratio: old position / new position ) * oldTime
		met.state=START;
		met.currBeat=1;
		refclockOverride( (ushort)(met.currMeasure/(float)prev)*getTime() );
	}
}
void transportFF(){
	
}
void setTempo( uchar tempo ){
	/* Set first and last beat */
	met.b[0]=			(TPS*120)/(tempo*met.tsig2/(float)2);
	met.b[met.tsig2-1]=	(TPS*120)/(float)(tempo/(float)2);
	uchar i;
	for( i=1; i<met.tsig2-1; i++ ){
		met.b[i]=((float)i+1)*met.b[0];
	}
	met.tempo=tempo;
}
void setTimeSignature( int tsig ){//THREE_FOUR, FOUR_FOUR, SIX_EIGHT
	switch (tsig){
		case THREE_FOUR:
			met.tsig1=3;
			met.tsig2=4;
			break;
		case FOUR_FOUR:
			met.tsig1=4;
			met.tsig2=4;
			break;
		case SIX_EIGHT:
			met.tsig1=6;
			met.tsig2=8;
			break;
		default:
			break;
	}
	setTempo( met.tempo );//update values
}

/* Menu manipulators */

void toggleBeepOn(){
	met.beepMaskPlay=( met.beepMaskPlay==0x18 )? 0x10 : 0x18;
}
void toggleDispMS(){
	met.dispTime=( met.dispTime==&dispMS )? &dispRaw : &dispMS; 
}

/* Accessors */
void getTimeSig_str( uchar buf[] ){
	buf[0]=met.tsig1+'0';
	buf[1]=':';
	buf[2]=met.tsig2+'0';
	buf[3]='\0';
}
void getTempo_str( uchar buf[] ){
	nToChars( (ushort)met.tempo, buf );
}
uchar getBeepOn(){
	return ( met.beepMaskPlay==0x18 )? 1 : 0;
}
uchar getDispMSOn(){
	return ( met.dispTime==&dispMS ); 
}
void getTimeMB( uchar buf[] ){
	uchar j=0;
	uchar nbuf[4];
	nToChars( met.currMeasure, nbuf );
	while( nbuf[j] ){
		buf[j]=nbuf[j];
		j++;
	}
	buf[j]=':';
	buf[j+1]=met.currBeat+'0';
	buf[j+2]='\0';
}
unsigned char getTempo(){ return met.tempo; }
unsigned char getTsig1(){ return met.tsig1; }
unsigned char getTsig2(){ return met.tsig2; }
float getBeatValue(){ return met.b[0]; }
unsigned char getCurrBeat(){ return met.currBeat; }
unsigned char getCurrMeasure(){ return met.currMeasure; }
