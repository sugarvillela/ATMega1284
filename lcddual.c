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
#include "UT.h"
#include "lcddual.h"

/*  A tool for message output to multiple screens. Can handle 8 messages: 4 per
	screen. You can set the timeout for the message to clear and allow the next 
	message to display.
	
	You can bypass queue for simple message display 
	
	When queue is bypassed you can override the bypass for err message.  After
	timeout, state is returned to bypass.
	
	This tool has more capacity than this program needs, but will be useful for 
	other projects.  As it is, only the startup screen uses the full queue mode
*/

/*===============LCD using message queue======================================*/

/* forward declare 'private' functions for managing the queue */
uchar incWrite( uchar wScreen );
uchar incRead();
uchar writeTo( uchar wScreen );
uchar readFrom();
void msgRead();
void msgDrop();

/*	Message Queue Globals:  Set LEN_Q = LEN_MSG * CAPACITY 
	Understand the above before changing */
#define LEN_Q		132
#define LEN_MSG		33
#define CAPACITY	4
#define NUM_SCRNS	2

/*	Screen specifies the current read/display screen; 
	Declare before LCD hardware functions to specify which screen
	the function displays to.
	Screen toggled by tick function 
	For write-related functions the screen is specified by the caller
*/
uchar screen; 
enum msg_states { WAIT, READ, HOLD, DROP };//period = 100 ms
	
typedef struct {
	char Q[LEN_Q];
	uchar msgDur[CAPACITY];
	/* read/write indexes and queue status */
	uchar enable;
	uchar lock;
	uchar iWrite;
	uchar iRead;
	uchar iDur;
	uchar numInQ;
	void (*dropFunct)();
	
	int state;
} messageQ;

messageQ msgQ[NUM_SCRNS];

/* LCD hardware uses 8 data pins and 4 control pins, as specified here */

#define DATA_BUS PORTC		// port connected to pins 7-14 of LCD display
#define CONTROL_BUS PORTD	// port connected to pins 4 and 6 of LCD disp.
#define RS0		4			// pin number of uC connected to pin 4 of LCD disp 0.
#define E0		5			// pin number of uC connected to pin 6 of LCD disp 0.
#define RS1		6			// pin number of uC connected to pin 4 of LCD disp 1.
#define E1		7			// pin number of uC connected to pin 6 of LCD disp 1.

/*===============Direct LCD write (not using queue)===========================*/

void LCD_ClearScreen( uchar wScreen ) {
	if( msgQ[wScreen].lock ){ return; }
	LCD_WriteCommand( 0x01, wScreen );
}
void LCD_init(void) {
	uchar wScreen;
	delay_ms( 500 );
	for( wScreen=0; wScreen<2; wScreen++ ){
		LCD_WriteCommand( 0x38, wScreen );
		LCD_WriteCommand( 0x06, wScreen );
		LCD_WriteCommand( 0x0f, wScreen );
		LCD_WriteCommand( 0x01, wScreen );
	}
	delay_ms( 50 );
}
void LCD_WriteCommand (uchar Command, uchar wScreen  ) {
	if( wScreen ){
		CLR_BIT(CONTROL_BUS,RS1);
		DATA_BUS = Command;
		SET_BIT(CONTROL_BUS,E1);
		asm("nop");
		CLR_BIT(CONTROL_BUS,E1);
	}
	else{
		CLR_BIT(CONTROL_BUS,RS0);
		DATA_BUS = Command;
		SET_BIT(CONTROL_BUS,E0);
		asm("nop");
		CLR_BIT(CONTROL_BUS,E0);
	}
	delay_ms( 10 ); // ClearScreen requires 1.52ms to execute
}
void LCD_WriteData( uchar Data, uchar wScreen ) {
	/* Writes a single character */
	if( msgQ[wScreen].lock ){ return; }
	if( wScreen ){
		SET_BIT(CONTROL_BUS,RS1);
		DATA_BUS = Data;
		SET_BIT(CONTROL_BUS,E1);
		asm("nop");
		CLR_BIT(CONTROL_BUS,E1);
	}
	else{
		SET_BIT(CONTROL_BUS,RS0);
		DATA_BUS = Data;
		SET_BIT(CONTROL_BUS,E0);
		asm("nop");
		CLR_BIT(CONTROL_BUS,E0);
	}
	delay_ms(1);
}
/*	LCD_Disp and LCD_DispNew do the same thing, except LCD_DispNew
	clears the screen first.
	Use LCD_Disp for writing multiple strings to a screen.
	You can write, change the cursor position and write again
	*/
void LCD_DispNew( uchar column, const uchar* string, uchar wScreen ) {
	if( msgQ[wScreen].lock ){ return; }
	LCD_ClearScreen( wScreen );
	uchar c = column;
	while(*string) {
		LCD_Cursor( c++, wScreen );
		LCD_WriteData( *string++, wScreen );
	}
}
void LCD_Disp( uchar column, const uchar* string, uchar wScreen ) {
	if( msgQ[wScreen].lock ){ return; }
	uchar c = column;
	while(*string) {
		LCD_Cursor( c++, wScreen );
		LCD_WriteData( *string++, wScreen );
	}
}
void LCD_Cursor( uchar column,  uchar wScreen ) {//set cursor position
	if( msgQ[wScreen].lock ){ return; }
	if ( column < 17 ) { // 16x1 LCD: column < 9
		// 16x2 LCD: column < 17
		LCD_WriteCommand( ( 0x80 + column - 1 ), wScreen );
	} 
	else {
		LCD_WriteCommand( ( 0xB8 + column - 9 ), wScreen );	// 16x1 LCD: column - 1
		// 16x2 LCD: column - 9
	}
}
void setScreen( uchar scr ){
	/* Sets the global index. Keep in mind, the tick function changes this */
	screen=scr;
}

/*===============Set up queue and enable=============================================*/

void msgInit(){
	uchar i, j;
	LCD_init();
	/* Already zero on system start, but if init called later, need to zero */
	for( i=0; i<NUM_SCRNS; i++ ){
		for( j=0; j<CAPACITY; j++ ){
			msgQ[i].msgDur[j]=0;
		}
		msgQ[i].enable=1;
		msgQ[i].lock=0;
		msgQ[i].iRead=(uchar)-LEN_MSG;
		msgQ[i].iWrite=(uchar)-LEN_MSG;
		msgQ[i].numInQ=0;
	}
	screen=0;
}
void msgQueueOn( uchar wScreen ){
	msgQ[wScreen].enable=1;
}
void msgQueueOff( uchar wScreen ){
	msgQ[wScreen].lock=0;
	msgClear( wScreen );
	msgQ[wScreen].enable=0;
}

/*===============You can set a function to run when a message times out==============*/

void msgBindDropEvent( uchar wScreen, void (*f)() ){
	msgQ[wScreen].dropFunct=f;
}
void msgUnbindDropEvent( uchar wScreen ){
	msgQ[wScreen].dropFunct=0;
}
void msgUnbindAll(){
	uchar i;
	for(i=0; i<NUM_SCRNS; i++){
		msgQ[i].dropFunct=0;
	}
}

/*===============Private functions that manage queue indexing=========================*/

/* Increment index by 1 message length and return index (wraps around on overrun) */
uchar incWrite( uchar wScreen ){
	msgQ[wScreen].iWrite+=LEN_MSG;
	if( msgQ[wScreen].iWrite==LEN_Q ){ 
		msgQ[wScreen].iWrite=0; 
	}
	return msgQ[wScreen].iWrite;
}
uchar incRead(){
	msgQ[screen].iRead+=LEN_MSG;
	if( msgQ[screen].iRead==LEN_Q ){ 
		msgQ[screen].iRead=0; 
	}
	return msgQ[screen].iRead;
}
/* Return index of next null write, or -1 if full */
uchar writeTo( uchar wScreen ){
	uchar count=0;
	while( msgQ[wScreen].Q[incWrite( wScreen )] ){
		if( CAPACITY == count++ ){
			return -1;
		}
	}
	return msgQ[wScreen].iWrite;
}
/* Return index of next non-null read, or -1 if empty */
uchar readFrom(){
	uchar count=0;
	while( !msgQ[screen].Q[incRead()] ){
		if( LEN_Q == count ){
			return -1;
		}
		count+=LEN_MSG;
	}
	return msgQ[screen].iRead;
}

/*===============Public:  Three ways of writing a message==============================*/

/* Standard way: finds an index and copies to queue */
void msgWrite( uchar wScreen, char buf[], uchar setTimeout ){
	uchar iTo, iFrom;
	/* Get an index to write to */
	if( ( iTo=writeTo( wScreen ) )==(uchar)-1 ){ 
		return; 
	}
	/* Set duration; FOREVER if arg is 0 */
	msgQ[wScreen].msgDur[iTo/LEN_MSG]=( setTimeout )? setTimeout: FOREVER;
	for ( iFrom=0; iFrom<LEN_MSG; iFrom++ ){//copy until null char found
		msgQ[wScreen].Q[iTo]=buf[iFrom];
		if( !buf[iFrom] ){
			break;
		}
		iTo++;
	}
	msgQ[wScreen].numInQ++;
}
/*	Convert a number to string and display in dec, hex or binary 
	Pass a string to include a label with the display */
void msgWrite_num( uchar wScreen, int format, char label[], ushort num, uchar setTimeout ){//label max 9 characters
	uchar i=0, j=0;
	uchar nbuf[17];
	char sbuf[33];
	
	while( label[i] ){
		sbuf[i]=label[i];
		i++;
	}
	while(i<16){
		sbuf[i]=' ';
		i++;
	}
	switch( format ){
		case NUM_HEX:
			nToHex( num, nbuf );
			break;
		case NUM_BIN:
			nToBin( num, nbuf );
			break;
		default:
			nToChars( num, nbuf );
			break;
	}
	while( nbuf[j] ){
		sbuf[i+j]=nbuf[j];
		j++;
	}
	sbuf[i+j]='\0';
	msgWrite( wScreen, sbuf, setTimeout );
}
/* Error message: override queue-off state */
void msgWriteErr( uchar wScreen, char buf[], uchar setTimeout ){
	/* Knocks all messages off the queue, grabs the lock and holds it until timeout 
		Sets msgRelease on timeout
		Overrides disabled state 
	*/
	msgClear( wScreen );
	msgQ[wScreen].enable=1;
	msgQ[wScreen].lock=1;
	msgQ[wScreen].state=WAIT;
	//msgBindDropEvent( wScreen, &msgRelease );
	msgWrite( wScreen, buf, setTimeout );
}
/* Private function that sends a queued message to screen */
void msgRead(){
	uchar iFrom, iTo, col=1;
	char buf[33];
	if( ( iFrom=readFrom() )==(uchar)-1 ){ return; }
	
	for ( iTo=0; iTo<LEN_MSG; iTo++ ){//copy
		buf[iTo]=msgQ[screen].Q[iFrom];
		if( !msgQ[screen].Q[iFrom] ){
			break;
		}
		iFrom++;
	}
	if( msgQ[screen].lock ){
		msgQ[screen].lock=0;
		LCD_DispNew( col, (const uchar* )buf, screen );
		msgQ[screen].lock=1;
	}
	else{
		LCD_DispNew( col, (const uchar* )buf, screen );
	}
	/* Set index where display duration is stored */
	msgQ[screen].iDur=msgQ[screen].iRead/LEN_MSG;
}


void msgDrop(){
	/* Deactivate current message; useful for clearing FOREVER message */
	msgQ[screen].Q[msgQ[screen].iRead]='\0';
	msgQ[screen].msgDur[msgQ[screen].iDur]=0;	
	msgQ[screen].numInQ--;
	LCD_ClearScreen( screen );
}
void msgClear( uchar wScreen ){
	/* Deactivate all messages */
	uchar i;
	for( i=0; i<LEN_Q; i+=LEN_MSG ){
		msgQ[wScreen].Q[i]='\0';
	}
	for( i=0; i<CAPACITY; i++ ){
		msgQ[wScreen].msgDur[i]=0;
	}
	msgQ[wScreen].iRead=(uchar)-LEN_MSG;
	msgQ[wScreen].iWrite=(uchar)-LEN_MSG;
	msgQ[wScreen].numInQ=0;
	LCD_ClearScreen( screen );
}
/* Functions to get queue status */
uchar msgIsLocked( uchar wScreen ){
	/* True when state machine is stuck in HOLD state by FOREVER */
	return msgQ[wScreen].msgDur[msgQ[wScreen].iRead]==FOREVER;
}
uchar fullQ( uchar wScreen ){
	return msgQ[wScreen].numInQ==CAPACITY;
}
uchar emptyQ(){
	return msgQ[screen].numInQ==0;
}
uchar numInQ( uchar wScreen ){
	return msgQ[screen].numInQ;
}
/* Call on init */	
void dispWelcomeScreen(){
	msgWrite( 0, "    Welcome!", MSG_LONG );
	msgWrite( 1, "  Programmable", MSG_SHORT );
	msgWrite( 1, "      Midi", MSG_SHORT );
	msgWrite( 1, "    Sequencer", MSG_SHORT );
}

/* State machine tick function */
void message_tick(){
	screen=!screen;//toggle index between 0 and 1

	if( !msgQ[screen].enable ){ return; }
	static uchar localTimer[NUM_SCRNS]={2,2};
	switch (msgQ[screen].state) {
		case WAIT://Stays here until there is a message
			if( !emptyQ() ){
				msgQ[screen].state = READ;
			}
			break;
		case READ://READ sends msg to display
			msgQ[screen].state = HOLD;
			break;
		case HOLD://HOLD manages the timeout
			if( msgQ[screen].msgDur[msgQ[screen].iDur]==FOREVER ){
				/*	If Forever is set, don't bother to count.
					Forever can be externally cleared using msgDrop() */
				return;
			}
			if( emptyQ() ){
				/* Someone cleared the messages */
				msgQ[screen].state = WAIT;
			}
			else if( msgQ[screen].msgDur[msgQ[screen].iDur] <= localTimer[screen] ){
				/* Message expiring */
				msgQ[screen].state = DROP;
			}
			localTimer[screen]++;
			break;
		case DROP://Either display another message or go to wait sate
			if( emptyQ() ){
				msgQ[screen].state = WAIT;
			}
			else{
				msgQ[screen].state = READ;
			}
			break;
		default:
			msgQ[screen].state = WAIT;
			break;
	}
	switch(msgQ[screen].state) {//action
		case WAIT:
		case HOLD:
			break;
		case READ:
			/* Inc read index, display message and set index to check dur */
			msgRead();
			break;			
		case DROP:
			/* Case: an err message is finishing; return to prev state */
			if( msgQ[screen].lock ){
				msgQ[screen].lock=0;
				msgQ[screen].enable=0;
				msgQ[screen].state=WAIT;
			}
			msgDrop();
			localTimer[screen]=2;			
			if( msgQ[screen].dropFunct ){
				msgQ[screen].dropFunct();
			}
			break;
		default:
			break;
	}
}

/*
*/