#include <stdio.h>
#include "lcddual.h"

/*	Screen specifies the current read/display screen; 
	Declare before LCD hardware functions to specify demux
	Toggled by tick function 
	Write screen specified by the caller to write-related functions
*/
uchar screen; 

/* 'private' functions */
void msgInit();
uchar incWrite( uchar wScreen );
uchar incRead();
uchar currWrite( uchar wScreen );
uchar currRead();
uchar writeTo( uchar wScreen );
uchar readFrom();
void msgRead();
void msgDrop();

/* Finds log 10 of input number */
unsigned char decPlaces( unsigned short in ){
	if( in==0 ){ return 1;}
	unsigned char i;
	for ( i=0; in && i< 0xFF; i++ ){
		in/= 10;
	}
	return i;
}
/* Finds log 16 of input number */
unsigned char hexPlaces( unsigned short in ){
	if( in==0 ){ return 1; }
	unsigned char i;
	for ( i=0; in && i< 0xFF; i++ ){
		in=in>>4;
	}
	return i;
}
/* For LCD display, returns input number in string form; buf size must be sufficient */
void nToChars( unsigned short in, unsigned char buf[] ){//buffer 6 for 16 bit range
	unsigned char n = decPlaces( in );
	unsigned short next, curr;
	char i;
	for ( i=n-1; i>=0; i-- ){
		next = in/10;
		curr = in - ( next*10 );
		//printf( "curr %d, %c\n", curr, curr+'0' );
		buf[i]=curr+'0';
		in = next;
	}
	buf[n]='\0';
}
void nToHex( unsigned short in, unsigned char buf[] ){//buffer 5 for 16 bit range
	unsigned char n = hexPlaces( in );
	char hex[]="0123456789ABCDEF";
	char i;
	//printf("nToHex: %d\n", in );
	for ( i=n-1; i>=0; i-- ){
		buf[i]=hex[in&0x0F];
		//printf("i=%d, n=%d\n", i, in );
		in=in>>4;
	}
	buf[n]='\0';
}
/* dummy functions */
void LCD_DisplayString( unsigned char col, const unsigned char* str ){
	printf( "LCD_DisplayString on screen %d\n",screen );
    printf( "%s\n",str );
}
void LCD_WriteData( unsigned char data ){}
void LCD_ClearScreen(){
	printf( "LCD_ClearScreen() %d\n", screen ); 
}

/* LCD display */
/*	Message Queue Globals:  Set LEN_Q = LEN_MSG * CAPACITY */
#define LEN_Q		132
#define LEN_MSG		33
#define CAPACITY	4
#define NUM_SCRNS	2
#define MSG_PERIOD	100
#define FOREVER		255

typedef struct {
	char Q[LEN_Q];
	uchar msgDur[CAPACITY];
	/* read/write indexes and queue status */
	uchar iWrite;
	uchar iRead;
	uchar iDur;
	uchar numInQ;
} messageQ;

messageQ msgQ[NUM_SCRNS];

	
void msgInit(){
	unsigned char i, j;
	/* Already zero on system start, but if init called later, need to zero */
	for( i=0; i<NUM_SCRNS; i++ ){
		for( j=0; j<CAPACITY; j++ ){
			msgQ[i].msgDur[j]=0;
		}
		msgQ[i].iRead=(uchar)-LEN_MSG;
		msgQ[i].iWrite=(uchar)-LEN_MSG;
		msgQ[i].numInQ=0;
	}
	screen=0;
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
	printf("msgWrite: iTo=%d: %s\n", iTo, buf );
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
	// buf[iTo]='\0';
	LCD_DisplayString( col, (const uchar* )buf );
	//printf( "%d, %c\n", starti, msgQ[screen].Q[starti] );
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
	LCD_ClearScreen();
}
void msgClear( uchar wScreen ){
	/* Deactivate all messages */
	uchar i;
	for( i=0; i<LEN_Q; i+=LEN_MSG ){
		msgQ[wScreen].Q[i]='\0';
		//printf("msgClear: i=%d\n", i);
	}
	for( i=0; i<CAPACITY; i++ ){
		msgQ[wScreen].msgDur[i]=0;
	}
	msgQ[wScreen].iRead=(uchar)-LEN_MSG;
	msgQ[wScreen].iWrite=(uchar)-LEN_MSG;
	msgQ[wScreen].numInQ=0;
	LCD_ClearScreen();
}
uchar fullQ( uchar wScreen ){
	return msgQ[wScreen].numInQ==CAPACITY;
}
uchar emptyQ(){
	//printf("emptyQ: msgQ[screen].numInQ=%d\n", msgQ[screen].numInQ );
	return msgQ[screen].numInQ==0;
}
uchar numInQ( uchar wScreen ){
	return msgQ[screen].numInQ;
}
/* Call on init */	
void dispWelcomeScreen( uchar wScreen ){
	char welcome[]="    Welcome!    ";
	msgWrite( wScreen, welcome, 30 );
}
/* State machine tick function */
enum msg_states { MSG_INIT, MSG_WAIT, MSG_READ, MSG_HOLD, MSG_DROP };//period = 100 ms
int message_tick( int state ){
	static uchar localTimer[NUM_SCRNS]={2,2};

	//printf("tick screen %d, state=%d\n", screen , state);
	switch (state) {//state
		case MSG_INIT:
			msgInit();
			
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
				//printf("screen %d: %d<=%d\n", screen, msgQ[screen].msgDur[msgQ[screen].iDur], localTimer[screen] );
				//printf("timeout screen %d\n", screen );
				/* Message expiring */
				state = MSG_DROP;
			}
			localTimer[screen]++;
			break;
		case MSG_DROP:
			state = ( emptyQ() )? MSG_WAIT : MSG_READ;
			break;
		default:
			state = MSG_INIT;
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
			//printf("dropping screen %d\n", screen );
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
	uchar i;
	int state0=-1;
	int state1=-1;
	for( i=0; i<25; i++ ){
		printf( "%d: state0=%d,state1=%d =====\n", i, state0, state1 );
		state0=message_tick( state0 );
		state1=message_tick( state1 );
		if( i==1){
			printf("write\n" );
			msgWrite( 0, "Screen 0 message A", 4 );
			msgWrite( 0, "Screen 0 message B", 2 );
			msgWrite( 1, "Screen 1 message A", 2 );
			msgWrite( 0, "Screen 0 message C", 4 );
			msgWrite( 1, "Screen 1 message B", 3 );	
		}
	}
}