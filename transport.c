//#include <stdio.h>
#include <avr/io.h>
#include "transport.h"
#include "refclock.h"
#include "lcddual.h"
#include "buttonbus.h"
#include "menu.h"//for binding stop press event to menu
#include "UT.h"

#define MAX_BEATS   8       //related to time signatures
#define TPS         1000L   //number of ticks per second (clock speed, period)
#define BEEP_DUR    40		//length of beeps (milliseconds) 
							//Set much lower than least expected beat duration
#define OUTPORT PORTB								

typedef struct {
	float b[MAX_BEATS];
	uchar tempo;                //beats per minute
	uchar tsig1;                //number of beats per bar
	uchar tsig2;                //bar length
	uchar currMeasure;          //for display
	uchar currBeat;
	uchar beepMaskPlay;			//0x18 for light and beep, 0x10 for light only
	uchar beepMaskRec;
	uchar beepMask;				
	int state;
	char flasher[5];			//Flash PLAY or REC
	//uchar recording;
} metronome;
metronome met;

int beepState;
uchar beepCount;
enum beepStates{ BEEPOFF, HION, HIOFF, LOON1, LOON2, LOOFF1, LOOFF2 };
int beep_tick( int state ){
	state=beepState;
	switch(state){
		case BEEPOFF:
			return state;
		/* High Cycle */
		case HION:
			OUTPORT |= met.beepMask;
			state=HIOFF;
			break;
		case HIOFF:
			OUTPORT &=(uchar)~met.beepMask;
			state=HION;
			break;
		/* Low Cycle */
		case LOON1:
			OUTPORT |= met.beepMask;
			state=LOON2;
			break;
		case LOON2:
			state=LOOFF1;
			break;
		case LOOFF1:
			OUTPORT &=(uchar)~met.beepMask;
			state=LOOFF2;
			break;
		case LOOFF2:
			state=LOON1;
			break;
	}
	if( ++beepCount>BEEP_DUR ){ 
		CLR_TWO( OUTPORT, 3 );
		return (beepState=BEEPOFF); 
	}
	return (beepState=state);
}

void beepHi(){
	beepState=HION;
	beepCount=0;
}
void beepLo(){
	beepState=LOON1;
	beepCount=0;
}

static char bufMB[10];				//measure:beat display
static char bufMSF[10];				//measure:time display

void dispMetStatus(){
	/* Right Screen */
	static uchar flash=1;
	getTimeMB( (uchar*)bufMB );
	getTimeMSF( bufMSF );
	if( flash ){
		simpleWrite_123( 1, bufMB, met.flasher, bufMSF );
	}
	else{
		simpleWrite_123( 1, bufMB, "    ", bufMSF );
	}
	flash=!flash;
	/* Left Screen */
}
enum metStates{ OFF, ON, ACCENT, BEAT, COUNT };

int metronome_tick( int state ){
	state=met.state;

	static uchar i=0;						//index of met.b for beat times
	static float count=0;				//for counting up to beat times
	switch (state){
		case BYP:
			return state;
		case OFF:
			break;
		case ON:
			i=0;
			count=0;
			state=ACCENT;
			break;
		case ACCENT:
			state=COUNT;
			break;
		case BEAT:
			state=COUNT;
			break;
		case COUNT:
			if( (count+5)>met.b[i] ){
				i++;
				if( i>=met.tsig1 ){
					state=ACCENT;
					met.currMeasure++;
				}
				else{
					state=BEAT;
				}
			}
			break;
		default:
			state=OFF;
			i=0;
			count=0;
			break;
	}
	switch (state){
		case OFF:
		case ON:
		case COUNT:
			break;
		case ACCENT:
			/* Update measures:beats for display */
			met.currBeat=1;
			/* Turn on high beep */
			beepHi();
			/* Display metronome time */
			dispMetStatus();
			/* Reset local vars for next measure */
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
			//transportInit();
			state=OFF;
			i=0;
			count=0;
			break;
	}
	count+=10;
	return ( met.state=state );
}
/* Mutators */
void transportInit(){
	/* set defaults */
	met.beepMaskPlay=0x10;
	met.beepMaskRec=0x18;
	met.tempo=120;
	setTimeSignature( FOUR_FOUR );
	/* Set up transport */
	met.state=OFF;
	beepState=BEEPOFF;
	/* Zero transport */
	zeroRefclock();
	met.currMeasure=1;
	met.currBeat=1;
	cp( met.flasher, "Stop" );
}
void startupDisplay(){
	msgUnbindDropEvent( 0 );
	transportOn();
	transportTop();
}
void transportOn(){
	bBusInit();
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
	msgQueueOn( 0 );
	msgWrite( 0, "Ready", MSG_SHORT );
	msgQueueOff( 1 );
    bindPressEvent( 0, &transportStop );
    bindPressEvent( 1, &transportRW );
    bindPressEvent( 2, &transportFF );
    bindPressEvent( 3, &play );
	bindDoubleEvent( 3, &record );
	bindHoldEvent( 0, &menuOn );
	//cp( met.flasher, "Stop" );
	//dispMetStatus();
}
void play(){
	msgWrite( 0, "Play", MSG_SHORT );
	met.beepMask=met.beepMaskPlay;
	cp( met.flasher, "Play" );
	if( met.state==OFF ){
		met.state=ON;
		refclockOn();
	}
}
void record(){
	msgWrite( 0, "Record", MSG_SHORT );
	met.beepMask=met.beepMaskRec;
	cp( met.flasher, "Rec " );
	bReset( 3 );//clear count so it doesn't call on every click
	if( met.state==OFF ){
		met.state=ON;
		refclockOn();
	}
}
void transportStop(){
	msgWrite( 0, "Stop", MSG_SHORT );
	met.state=OFF;
	refclockOff();
	cp( met.flasher, "Stop" );
	OUTPORT &=0xE7;
	dispMetStatus();
	bReset( 3 );
}
void transportTop(){
	zeroRefclock();
	met.currMeasure=1;
	met.currBeat=1;
	cp( met.flasher, "Stop" );
	dispMetStatus();
}
void transportRW(){
	
}
void transportFF(){
	
}
void setTempo( uchar tempo ){
	/* Set first and last beat */
	met.b[0]=(TPS*120)/(tempo*met.tsig2/(float)2);
	met.b[met.tsig2-1]=(TPS*120)/(float)(tempo/(float)2);
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
void toggleBeepOn(){
	met.beepMaskPlay=( met.beepMaskPlay==0x18 )? 0x10 : 0x18;
}
void beepPref( uchar onOff ){
	met.beepMaskPlay=( onOff )? 0x18 : 0x10;
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
