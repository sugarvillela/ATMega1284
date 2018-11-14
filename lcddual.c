#include <avr/io.h>
#include <avr/interrupt.h>
#include "UT.h"
#include "lcddual.h"

/*  A tool for message output to multiple screens.
    
    IMPORTANT: the way tick-sharing works is you make a task for each screen.
    The tick function increments the screen just before it returns so the next
    tick accesses the next screen.  Make sure NUM_SCRNS = number of tasks that
    use the tick function
*/

/* =============================LCD functions=============================*/
enum msgModes{ MSG_QUEUE, MSG_SIMPLE };



/*-------------------------------------------------------------------------*/
#define DATA_BUS PORTC		// port connected to pins 7-14 of LCD display
#define CONTROL_BUS PORTD	// port connected to pins 4 and 6 of LCD disp.
#define RS0		4			// pin number of uC connected to pin 4 of LCD disp 0.
#define E0		5			// pin number of uC connected to pin 6 of LCD disp 0.
#define RS1		6			// pin number of uC connected to pin 4 of LCD disp 1.
#define E1		7			// pin number of uC connected to pin 6 of LCD disp 1.
/*-------------------------------------------------------------------------*/
/*==========================================================================*/
/*	Screen specifies the current read/display screen; 
	Declare before LCD hardware functions to specify demux
	Toggled by tick function 
	Write screen specified by the caller to write-related functions
*/
uchar screen; 
/*==========================================================================*/
void LCD_ClearScreen( uchar wScreen ) {
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
void LCD_DispNew( uchar column, const uchar* string, uchar wScreen ) {
	LCD_ClearScreen( wScreen );
	uchar c = column;
	while(*string) {
		LCD_Cursor( c++, wScreen );
		LCD_WriteData( *string++, wScreen );
	}
}
void LCD_Disp( uchar column, const uchar* string, uchar wScreen ) {
	uchar c = column;
	while(*string) {
		LCD_Cursor( c++, wScreen );
		LCD_WriteData( *string++, wScreen );
	}
}
void LCD_Cursor( uchar column,  uchar wScreen ) {
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
	screen=scr;
}
/* =============================msg functions=============================*/
/* 'private' functions */

uchar incWrite( uchar wScreen );
uchar incRead();
uchar currWrite( uchar wScreen );
uchar currRead();
uchar writeTo( uchar wScreen );
uchar readFrom();
void msgRead();
void msgDrop();

/* LCD display */
/*	Message Queue Globals:  Set LEN_Q = LEN_MSG * CAPACITY */
#define LEN_Q		132
#define LEN_MSG		33
#define CAPACITY	4
#define NUM_SCRNS	2

typedef struct {
	uchar mode;		//Need this to keep from bogging down on busy messages
	char Q[LEN_Q];
	uchar msgDur[CAPACITY];
	/* read/write indexes and queue status */
	uchar iWrite;
	uchar iRead;
	uchar iDur;
	uchar numInQ;
	void (*dropFunct)();
} messageQ;

messageQ msgQ[NUM_SCRNS];

	
void msgInit(){
	uchar i, j;
	LCD_init();
	/* Already zero on system start, but if init called later, need to zero */
	for( i=0; i<NUM_SCRNS; i++ ){
		msgQ[i].mode=MSG_QUEUE;
		for( j=0; j<CAPACITY; j++ ){
			msgQ[i].msgDur[j]=0;
		}
		msgQ[i].iRead=(uchar)-LEN_MSG;
		msgQ[i].iWrite=(uchar)-LEN_MSG;
		msgQ[i].numInQ=0;
	}
	screen=0;
}
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
/* Increment index by 1 message length and return index */
uchar incWrite( uchar wScreen ){
	msgQ[wScreen].iWrite+=LEN_MSG;
	//printf("incWrite: msgQ[wScreen].iWrite=%d\n", msgQ[wScreen].iWrite );
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
/* Return index without increment */
uchar currWrite( uchar wScreen ){
	return msgQ[wScreen].iWrite;
}
uchar currRead(){
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
void msgWrite_stack( uchar wScreen, char buf1[], char buf2[], uchar setTimeout ){
	uchar i=0, j=0;
	char sbuf[33];
	while( buf1[i] ){
		sbuf[i]=buf1[i];
		i++;
	}
	while(i<16){
		sbuf[i]=' ';
		i++;
	}
	while( buf2[j] ){
		sbuf[i+j]=buf2[j];
		j++;
	}
	sbuf[i+j]='\0';
	msgWrite( wScreen, sbuf, setTimeout );
}
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
void msgQueueOn( uchar wScreen ){
	msgQ[wScreen].mode=MSG_QUEUE;
}
void msgQueueOff( uchar wScreen ){
	msgQ[wScreen].mode=MSG_SIMPLE;
}
void simpleWrite( uchar wScreen, char buf[] ){
	LCD_DispNew( (uchar)1, (const uchar* )buf, screen );
}
void simpleWrite_num( uchar wScreen, int format, ushort num ){
	uchar nbuf[17];
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
	LCD_ClearScreen( wScreen );
	LCD_Disp( (uchar)1, (const uchar*)nbuf, wScreen );
}
void simpleWrite_float( uchar wScreen, float num ){
	uchar nbuf[17];
	//nToChars( (ushort)(num*100), nbuf );
	fToChars( num, nbuf );
	LCD_DispNew( (uchar)1, (const uchar*)nbuf, wScreen );
}
void simpleWrite_13( uchar wScreen, char buf1[], char buf2[] ){
	//LCD_ClearScreen( wScreen );
	LCD_Disp( (uchar)1, (const uchar* )buf1, wScreen );
	LCD_Disp( (uchar)17, (const uchar* )buf2, wScreen );
}
void simpleWrite_123( uchar wScreen, char buf1[], char buf2[], char buf3[] ){
	//LCD_ClearScreen( wScreen );
	LCD_Disp( (uchar)1, (const uchar* )buf1, wScreen );
	LCD_Disp( (uchar)9, (const uchar* )buf2, wScreen );
	LCD_Disp( (uchar)17, (const uchar* )buf3, wScreen );
}
void simpleWrite_1234( uchar wScreen, char buf1[], char buf2[], char buf3[], char buf4[] ){
	//LCD_ClearScreen( wScreen );
	LCD_Disp( (uchar)1, (const uchar* )buf1, wScreen );
	LCD_Disp( (uchar)9, (const uchar* )buf2, wScreen );
	LCD_Disp( (uchar)17, (const uchar* )buf3, wScreen );
	LCD_Disp( (uchar)25, (const uchar* )buf4, wScreen );
}
void simpleClear( uchar wScreen ){
	if( msgQ[wScreen].mode==MSG_SIMPLE){
		LCD_ClearScreen( wScreen );
	}
}

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
	LCD_DispNew( col, (const uchar* )buf, screen );
	/* Set index where display duration is stored */
	msgQ[screen].iDur=msgQ[screen].iRead/LEN_MSG;
}
/* "Public" functions */
uchar msgIsLocked( uchar wScreen ){
	/* True when state machine is stuck in HOLD state by FOREVER */
	return msgQ[wScreen].msgDur[msgQ[wScreen].iRead]==FOREVER;
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
void dispWelcomeScreen( uchar wScreen ){
	msgWrite( 0, "    Welcome!", MSG_LONG );
	msgWrite( 1, "  Programmable", MSG_SHORT );
	msgWrite( 1, "      Midi", MSG_SHORT );
	msgWrite( 1, "    Sequencer", MSG_SHORT );
}
/* State machine tick function */
enum msg_states { MSG_INIT, MSG_WAIT, MSG_READ, MSG_HOLD, MSG_DROP };//period = 100 ms
int message_tick( int state ){
	if( msgQ[screen].mode==MSG_SIMPLE ){ return state; }
	static uchar localTimer[NUM_SCRNS]={2,2};
	switch (state) {//state
		case BYP:
			return state;
		case MSG_INIT:
			state = MSG_WAIT;
			break;
		case MSG_WAIT:
			if( !emptyQ() ){
				state = MSG_READ;
			}
			break;
		case MSG_READ:
			state = MSG_HOLD;
			break;
		case MSG_HOLD:
			if( msgQ[screen].msgDur[msgQ[screen].iDur]==FOREVER ){
				/* Forever can be externally cleared using msgDrop() */
				return state;
			}
			if( emptyQ() ){
				/* Someone cleared the messages */
				state = MSG_WAIT;
			}
			else if( msgQ[screen].msgDur[msgQ[screen].iDur] <= localTimer[screen] ){
				/* Message expiring */
				state = MSG_DROP;
			}
			localTimer[screen]++;
			break;
		case MSG_DROP:
			if( emptyQ() ){
				state = MSG_WAIT;
				if( msgQ[screen].dropFunct ){
					msgQ[screen].dropFunct();
				}
			}
			else{
				state = MSG_READ;
			}
			break;
		default:
			state = MSG_WAIT;
			break;
	}
	switch(state) {//action
		case MSG_INIT:
		case MSG_WAIT:
		case MSG_HOLD:
			break;
		case MSG_READ:
			/* Inc read index, display message and set index to check dur */
			msgRead();
			break;			
		case MSG_DROP:
			msgDrop();
			localTimer[screen]=2;
			break;
		default:
			break;
	}
	screen=!screen;
	return state;
}

void lcdTest() {// MSG_INIT, MSG_WAIT, MSG_READ, MSG_HOLD, MSG_DROP
	//uchar i;
	//int state0=-1;
	//int state1=-1;
	//for( i=0; i<25; i++ ){
		//printf( "%d: state0=%d,state1=%d =====\n", i, state0, state1 );
		//state0=message_tick( state0 );
		//state1=message_tick( state1 );
		//if( i==1){
			//printf("write\n" );
			//msgWrite( 0, "Screen 0 message A", 4 );
			//msgWrite( 0, "Screen 0 message B", 2 );
			//msgWrite( 1, "Screen 1 message A", 2 );
			//msgWrite( 0, "Screen 0 message C", 4 );
			//msgWrite( 1, "Screen 1 message B", 3 );	
		//}
	//}
}