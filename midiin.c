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
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "midiin.h"
#include "lcddual.h"
#include "refclock.h"
#include "buttonbus.h"
#include "transport.h"//define TPS
#include "readWrite.h"
#include "UT.h"

////////////////////////////////////////////////////////////////////////////////
//Functionality - Initializes TX and RX on PORT D
//Parameter: usartNum specifies which USART is being initialized
//			 If usartNum != 1, default to USART0
//Returns: None
void initUSART(unsigned char usartNum)
{
	if (usartNum != 1) {
		// Turn on the reception circuitry of USART0
		// Turn on receiver and transmitter
		// Use 8-bit character sizes
		UCSR0B |= (1 << RXEN0)  | (1 << TXEN0);
		UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);
		//UCSR0B |= (1 << RXCIE0);//bit 7  // TXCIE0 is bit 6 for transmit
		// Load lower 8-bits of the baud rate value into the low byte of the UBRR0 register
		UBRR0L = BAUD_PRESCALE;
		// Load upper 8-bits of the baud rate value into the high byte of the UBRR0 register
		UBRR0H = (BAUD_PRESCALE >> 8);
		
	}
	else {
		// Turn on the reception circuitry for USART1
		// Turn on receiver and transmitter
		// Use 8-bit character sizes
		UCSR1B |= (1 << RXEN1)  | (1 << TXEN1);
		UCSR1C |= (1 << UCSZ10) | (1 << UCSZ11);
		// Load lower 8-bits of the baud rate value into the low byte of the UBRR1 register
		UBRR1L = BAUD_PRESCALE;
		// Load upper 8-bits of the baud rate value into the high byte of the UBRR1 register
		UBRR1H = (BAUD_PRESCALE >> 8);
		
	}
}

/*========================================================================================
	Interrupt-based USART
==========================================================================================*/
/*========================================================================================
	Record, Play, Overdub, Display and Time-Manipulate incoming midi data
==========================================================================================*/
void merge();

#define DISP_SKIP       10           //eventUpSkip() 

enum midiStates{ OFF, REC, PLAY, DUB, WAIT };
enum mStates{ SB, P1, P2, SKIP2, SKIP1 };

typedef struct {
	ushort t;
	uchar p1;
	uchar p2;
} midiNote;

typedef struct {
	midiNote d[MIDI_SIZE];
	/* record positions */
	ushort rBot;                    // set to last rCur
	ushort rCur;
	uchar full;                     // true when rCur>=MIDI_SIZE
	ushort rTop;                    // updated to last rCur on stop
	
	/* play positions */
	ushort pCur;                    // set to 0 or by RW FF
	uchar pDone;                    // true when pCur==pLim
	
	/* echo positions */
	ushort eBot;                    // set to last rCur
	ushort eCur;                    // set to eBot
	uchar eDone;                    // true when pCur==rCur

	/* display position */
	ushort dCur;                    // controlled by eventUp, eventDown
	
	/* state machines */
	int state;						// Midi modes: OFF, REC, PLAY, DUB, WAIT
	int rState;						// record SM
	int pState;						// play SM
	int eState;						// echo SM
	uchar sState;					// iterator/loader
	uchar echo;						// For toggle between echo and play SM's
	
	/* Previous status byte*/
	uchar rxSH;                     // for receive Midi Shorthand
	uchar txSH;                     // for transmit Midi Shorthand
	
	void (*functOff)();
	ushort currTime;                /* managed by refclock via midi_tick */
	
} midiStruct;
midiStruct M;

/*===============Recording and Playback=============================================*/

void midiInit(){
	initUSART(0);	// initializes USART0
	initUSART(1);
	
	M.rBot=0;                   // record positions
	M.rCur=0;
	M.rTop=0;
	M.full=0;
	
	M.pCur=0;                   // play position and status
	M.pDone=0;
	
	M.eBot=0;                   // echo position and status
	M.eCur=0;
	M.eDone=0;

	M.dCur=0;					//display position
	
	M.state=OFF;				//state machines
	M.rState=SB;
	M.pState=SB;
	M.eState=SB;
	M.currTime=0;
	
	M.sState=0;
}
void midiRec(){
	if( M.full ){
		msgWriteErr( 0, "Full!", MSG_SHORT );
		return;
	}
	if( M.rCur>0 ){
		M.state=DUB;
		M.pState=SB;
		M.pDone=0;
		SET_BIT( UCSR1B, UDRIE1 );	//USART 1 transmit interrupt on
		SET_BIT( UCSR1A, UDRE1 );	//Make an interrupt
	}
	else{
		M.state=REC;
		M.pDone=1;
	}
	M.rState=SB;
	M.rxSH=0x90;
	
	M.eState=SB;
	M.eDone=0;
	M.echo=1;
	M.txSH=2;
	SET_BIT( UCSR0B, RXCIE0 );		//USART 0 receive interrupt on
}
void midiPlay(){
	M.echo = 0;	//no echo during playback
	M.eDone=1;
	if( M.rTop>0 ){
		M.state=PLAY;
		M.pState=SB;
		M.pDone=0;
		M.txSH=2;
		M.echo=0;
		SET_BIT( UCSR1B, UDRIE1 );	//USART 1 transmit interrupt on
		SET_BIT( UCSR1A, UDRE1 );	//Make an interrupt
	}
	else{
		M.state=WAIT;
		M.pDone=1;
		CLR_BIT( UCSR1B, UDRIE1 );	//USART 1 transmit interrupt off
	}
}
void midiOff(){
	CLR_BIT( UCSR0B, RXCIE0 );	//record off
	CLR_BIT( UCSR1B, UDRIE1 );	//play off
	M.state=OFF;
	if( M.rBot<M.rCur ){
		M.rTop=( M.rState==SB )? M.rCur: M.rCur+1;//if it finished, it incremented
		merge();
		M.rBot=M.rTop;
		M.eBot=M.rTop;
		M.eCur=M.rTop;
	}
}
void midiTop(){
	M.pCur=0;
	M.currTime=0;
}
void midiSeek( unsigned short t ){
	M.pCur=0;
}

void midi_tick( ushort t ){
	/*	This function is called by refclock to wake up the midi if
		it has finished and shut itself off */
	M.currTime=t;
	switch(M.state){
	    case WAIT:
	        return;
        case PLAY:
    	    M.pDone=0;
    		M.eDone=1;
    		M.echo=0;
			SET_BIT( UCSR1B, UDRIE1 );	//USART 1 transmit interrupt on
			SET_BIT( UCSR1A, UDRE1 );	//Make an interrupt
        	return;
        case REC:
    	    M.pDone=1;
    		M.eDone=0;
    		M.echo=1;
			SET_BIT( UCSR1B, UDRIE1 );	//For echo
			SET_BIT( UCSR1A, UDRE1 );	//Make an interrupt
        	return;
        case DUB:
    	    M.pDone=0;
    		M.eDone=0;
    		M.echo=1;
			SET_BIT( UCSR1B, UDRIE1 );	//For play and echo
			SET_BIT( UCSR1A, UDRE1 );	//Make an interrupt
        	return;
	}
}

ISR( USART0_RX_vect ) {//receive interrupt
	/* prevState is for saving state while handling a message */
	static uchar udr, prevState;
	while ( !(UCSR0A & (1 << RXC0)) ); // Wait for data to be received
	udr=UDR0;//get from register
    if( udr&0x80 ){//status byte! What kind of message?
	    switch(udr&0xF0){
			/* Only accepting on and off messages 90 and 80 */
		    case 0x90:	
		    case 0x80:
				M.rState=SB;//override state machine order
				break;		//run the function; all others return
			/* All these types will be ignored for varying amounts of time */
		    case 0xA0:
		    case 0xB0:
		    case 0xE0:
				prevState=M.rState;
				M.rState=SKIP2;//skip next 2 bytes
				return;
		    case 0xC0:
		    case 0xD0:
				prevState=M.rState;
				M.rState=SKIP1;//skip next byte
				return;
		    default:// 0xF? messages are system-exclusive: check low nibble
				prevState=M.rState;
				M.rState=(udr&0x01)? SKIP1 : SKIP2;//odd skips 1, even skips 2
				return;
	    }
    }
    switch( M.rState ){
	    case SB://expect status byte or data1 if shorthand
			M.d[M.rCur].t=M.currTime;
			if( udr&0x80 ){			//MSB == 1 is a status message
				M.rxSH=udr;			//Save for shorthand
				M.d[M.rCur].p1=( udr&0x10 )? 0x80 : 0;//saving on/off as MSB 1 or 0 in data byte
				M.rState=P1;
			}
			else{                   //MSB == 0 is data: must be shorthand
				M.d[M.rCur].p1=(M.rxSH&0x90)? 0x80|udr : udr;
				M.rState=P2;
			}
			return;
	    case P1:
			if( udr > 11 ){//ignore notes below range
				M.d[M.rCur].p1 |= udr;
			}
			M.rState=P2;
			return;
			case P2:
			if( udr < 5 ){//ignore velocities below range
				//M.d[M.rCur].p2 = 0;
				M.d[M.rCur].p1 &= 0x7F;//change this to a stop message
			}
			else{
				M.d[M.rCur].p2 = udr;
			}
			M.rState=SB;
			/* Open a new note struct */
			M.rCur++;
			if( M.rCur>=MIDI_SIZE ){
				M.full=1;
				midiOff();
				return;
			}
			return;
	    case SKIP2://counts down to SKIP1 then back to prev
			M.rState=SKIP1;
			return;
	    case SKIP1:
			M.rState=prevState;
			return;
    }
}
ISR( USART1_UDRE_vect ) {//transmit interrupt
    if( M.eDone && M.pDone ){							//nothing to send
	    CLR_BIT( UCSR1B, UDRIE1 );//USART 1 transmit interrupt off
	    return;
    }
    /* 'echo' toggles between echo and play SM's */
    if( M.echo && !M.eDone ){// M.echo
	    if(											//This is here to keep echo within boundary
	     	M.eCur >= M.rCur ||						// Stop echo at previous record stop
	     	M.d[M.eCur].t > M.currTime				// or wait for refclock to catch up
	    ){
	     	M.eDone=1;
	     	M.echo=( M.state!=REC );				// If DUB, hand control back, else keep
	     	return;
	    }
	    switch( M.eState ){//see
		    case SB:								//CASE: status byte
				if( (M.d[M.eCur].p1&0x80)==M.txSH ){//If same status as previous, skip
					UDR1=M.d[M.eCur].p1&0x7F;		//status and send first data byte
					M.eState=P2;
				}
				else if( M.d[M.eCur].p1&0x80 ){		//MSB of p1 is 1
					UDR1=0x90;						//send ON
					M.txSH=0x80;
					M.eState=P1;
				}
				else{								//MSB of p1 is 0
					UDR1=0x80;						//send OFF
					M.txSH=0;
					M.eState=P1;
				}
				return;
		    case P1:								//CASE: send note number
				UDR1=M.d[M.eCur].p1&0x7F;			//delete status encoded in MSB
				M.eState=P2;
				return;
		    case P2:								//CASE: send velocity
				UDR1=M.d[M.eCur].p2;
				M.eState=SB;						//Advance counter and reset
				M.eCur++;
				if( M.eCur>=M.rCur ){
					M.eDone=1;
				}
				M.echo=( M.state==REC || M.pDone );	//if not REC and play not done, hand control back
				return;
	    }
    }
    else if( !M.pDone ){
	    if(											//This is here to prevent spurious notes
			M.pCur >= M.rTop ||						// Stop playback at previous record stop
			M.d[M.pCur].t > M.currTime				// or wait for refclock to catch up
	    ){
		    M.pDone=1;
		    M.echo=( M.state!=PLAY );				// If REC or DUB, hand control back, else keep
		    return;
	    }
	    switch( M.pState ){//identical to switch above, except where noted
		    case SB:
				if( (M.d[M.pCur].p1&0x80)==M.txSH ){
					UDR1=M.d[M.pCur].p1&0x7F;
					M.pState=P2;
				}
				else if( M.d[M.pCur].p1&0x80 ){
					UDR1=0x90;
					M.txSH=0x80;
					M.pState=P1;
				}
				else{
					UDR1=0x80;
					M.txSH=0;
					M.pState=P1;
				}
				return;
		    case P1:
				UDR1=M.d[M.pCur].p1&0x7F;
				M.pState=P2;
				return;
		    case P2:
		    UDR1=M.d[M.pCur].p2&0x7F;
				M.pState=SB;
				M.pCur++;
				M.echo=( M.state!=PLAY );			// If REC or DUB, hand control back, else keep
				return;
	    }
    }
}

void midiClockOverride( unsigned short t ){
	/* For rewinding: this feature is not fully implemented */
	M.currTime=t;
	//M.pCur=t;
}
uchar midiFull(){//exceeded array size
	return M.full;
}
/*===============Event List Display=================================================*/

void eventDispOn(){
	bBusInit();
	msgQueueOff( 0 );
	msgQueueOff( 1 );
	M.dCur=0;
	eventDispUpdate();
	bindPressEvent( 3, &eventUpSkip	);
	bindPressEvent( 1, &eventUp		);
	bindPressEvent( 2, &eventDown	);
	bindPressEvent( 0, &eventDownSkip );
	bindHoldEvent(	0, &eventDispOff );
}
void eventDispOff(){
	unbindAll();
	if( M.functOff ){
		 M.functOff();
	}
}
void noteMap( uchar n, uchar buf[] ){
	/* converts scientific musical notation to note-octave notation */
	static char notes[]="C#D#EF#G#A#B";
	uchar i=0, octave=(n>11)? n/12-1 : 0;//disallow negative octave
	n=n%12;

	if( notes[n]=='#' ){
		buf[i++]=notes[n-1];
		buf[i++]='#';
	}
	else{
		buf[i++]=notes[n];
	}
	buf[i++]=octave+'0';
	buf[i]='\0';
}
void eventDispUpdate(){
    uchar p1=M.d[M.dCur].p1&0x7F;
    uchar p2=M.d[M.dCur].p2;
    ushort t=M.d[M.dCur].t;
	uchar val[6];
	
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
	
    if( M.d[M.dCur].p1&0x80 ){
		LCD_Disp( 1, (const uchar*)">On", 0 );
    }
    else{
		LCD_Disp( 1, (const uchar*)">Off", 0 );
    }
	noteMap( p1, val );
	LCD_Disp( 8, val, 0 );
	nToHex( p2, val );
	LCD_Disp( 12, val, 0 );
	nToChars( t, val );
	LCD_Disp( 1, val, 1 );

    if( (M.dCur+1)<M.rTop ){
	    p1=M.d[M.dCur+1].p1&0x7F;
	    p2=M.d[M.dCur+1].p2;
	    t=M.d[M.dCur+1].t;
	
		if( M.d[M.dCur+1].p1&0x80 ){
			LCD_Disp( 18, (const uchar*)"On", 0 );
		}
		else{
			LCD_Disp( 18, (const uchar*)"Off", 0 );
		}
		noteMap( p1, val );
		LCD_Disp( 24, val, 0 );
		nToHex( p2, val );
		LCD_Disp( 28, val, 0 );
		nToChars( t, val );
		LCD_Disp( 17, val, 1 );
    }
}
void eventUp(){
    M.dCur--;
    if( M.dCur==(ushort)-1){
        M.dCur=0;
    }
    eventDispUpdate();
}
void eventDown(){
    M.dCur++;
    if( M.dCur>=M.rTop){
        M.dCur=M.rTop-1;
    }
    eventDispUpdate();
}
void eventUpSkip(){
    M.dCur-=DISP_SKIP;
    if( M.dCur>M.rTop){//unsigned negative is large positive
        M.dCur=0;
    }
    eventDispUpdate();
}
void eventDownSkip(){
    M.dCur+=DISP_SKIP;
    if( M.dCur>=M.rTop){
        M.dCur=M.rTop-1;
    }
    eventDispUpdate();
}
void eventStatus( uchar buf[] ){//fills in value field in menu->events
	if( M.rTop==0 ){
		cp( (char*)buf, "Empty" );
	}
	else if( M.full ){
		cp( (char*)buf, "Full" );
	}
	else{
		nToChars( M.rTop, buf );
	}
}
unsigned char haveEvents(){
	return ( M.rTop )? 1 : 0;
}
void testFill(){
	ushort t=100, i;
	uchar note=0x24, v=127;
	for( i=0;i<=12;i++){
		if( !M.full ){
			if( i%2==0 ){
				note+=7;
				M.d[M.rCur].p1 = note | 0x80;
			}
			else{
				M.d[M.rCur].p1 = note;
			}
			M.d[M.rCur].t=t;
			M.d[M.rCur].p2=v;
			M.rCur++;
			M.full=(M.rCur>=MIDI_SIZE);
		}
		t+=120;
	}
	M.rTop=M.rCur;
}

/*===============Data Manipulation==================================================*/

void writeNewTempo( uchar oldTempo, uchar newTempo ){
	/* changing tempo requires rewriting timestamp of every note */
    float ratio=oldTempo/(float)newTempo;
    ushort i;
    ushort prevOld=0, prevNew=0;
    progress( 0, M.rCur );
    for( i=0;i<M.rCur;i++){
        progress( 1, i );
        if(M.d[i].t==prevOld){          //skip repeated items
            M.d[i].t=prevNew;
            continue;
        }
        prevOld=M.d[i].t;               //save old value
        M.d[i].t=(ushort)M.d[i].t*ratio;//calculate new value
        prevNew=M.d[i].t;               //save new value
    }
    progress( 2, 0 );
}
void quantize( uchar tempo, uchar tsig2, uchar res ){
    if(!M.rTop){return;}
	progress( 0, M.rCur );
    /* Calculate a beat value based on tempo
        tsig2 is number of beats in a bar
        res is the resolution 1/8. 1/4 etc 
        Function assumes data is sorted by time
    */
	
    /* Value of one beat in transport ticks*/
    float b=24000/(float)(tempo*tsig2);
    b/=(res/tsig2);//correct for 1/4, 1/8, 1/16 (1/4 is no change)
	
    /* Current and previous positions; halfway points to prev and next */
    float bCurr=b, bPrev=0;
    float bMinus=b/2, bPlus=1.5*b;
	
    /* More accurate to mult than to add; inc mult after each calulation */
    ushort mult=1;
	unsigned short i=0;
	while( 1 ){
		if( M.d[i].t>=bMinus ){
			if( M.d[i].t<=bPlus ){
				M.d[i].t=bCurr;
				do{
					i++;
					if(i>=M.rTop){
						progress( 2, 0 );
						return;
					}
				}
				while( !(M.d[i].p1&0x80) );
				progress( 1, i );
			}
			else{
				mult++;
				bCurr=mult*b;
				bMinus=(bPrev+bCurr)/2;
				bPlus=(bCurr+bCurr+b)/2;
				bPrev=bCurr;
			}
		}
		else {
			M.d[i].t=0;
			i++;
			if(i>=M.rTop){
				break;
			}
		}
	}
	progress( 2, 0 );
}
void merge(){
	/*	Previously recorded data is at beginning of array
		Newly recorded data is at end (out of order).
		Sort by timestamp, leveraging the fact that previous
		and new are ordered with respect to themselves. 
		O(n^2), regardless.  Must be done 'in place' due to
		space constraints
	*/
    if( M.rBot==0 ){ //no data, no merge
		return; 
	}
    ushort i, r, p=(ushort)-1;//r for recorded, p for previous=65535
    midiNote temp;
	progress( 0, M.rTop-M.rBot );
    for( r=M.rBot; r<M.rTop; r++ ){
        progress( 1, r );
		/* Parse previous, find previous > new */
        do{ p++; } while( p<r && M.d[p].t <= M.d[r].t );
		/* Save data at r */
        temp.p1=M.d[r].p1;
        temp.p2=M.d[r].p2;
        temp.t=M.d[r].t;
		/* Slide everything up to make room */
        for( i=r; i>p; i-- ){
            M.d[i].p1=M.d[i-1].p1;
            M.d[i].p2=M.d[i-1].p2;
            M.d[i].t=M.d[i-1].t;
        }
		/* Put the data from r into the open spot at p */
        M.d[p].p1=temp.p1;
        M.d[p].p2=temp.p2;
        M.d[p].t=temp.t;
    }
	progress( 2, 0 );
}

/*===============Logistics==========================================================*/

void midiBindOffEvent( void (*f)() ){
	M.functOff=f;
}
void midiUnbindAll(){
	M.functOff=0;
}

/*===============Save data to eeprom================================================*/
/*	Saving song to file is a little complicated, involving song.c, midi.c and 
	readWrite.c (some refactoring would help)
	song.c calls readwrite to save header info: title and tempo 
	midi.c calls readWrite to save data size, then writes data */
void midiToFile(){
	fWriteSize( M.rTop<<2 );
	eeprom_write_block( 
		(const void*)&M.d[0],
		(void*)32,
		(size_t)M.rTop*4
	);
}
void midiFromFile(){
	ushort size=fReadSize();
	eeprom_read_block(
		(void*)&M.d[0],
		(const void*)32,
		(size_t)size
	);
	midiInit();
	M.rTop=size>>2;
	M.rBot=M.rTop;
	M.eBot=M.rTop;
	M.eCur=M.rTop;
}

/*===============Iterator and Loader for file save==================================*/
/*	I later replaced this with the simpler functions above, using 
	avr eeprom_write_block() */
/* Event iterator */
void itrReset(){
	M.dCur=0;
	M.sState=0;
}
unsigned char itrDone(){
	return M.dCur>=M.rTop;
}
unsigned short itrNext(){
	unsigned short temp;
	if( M.sState ){         // state 1
		temp=M.d[M.dCur].t;
		M.dCur++;
	}
	else{                   // state 0
		temp=M.d[M.dCur].p1;
		temp=temp<<8;
		temp |= M.d[M.dCur].p2;
	}
	M.sState=!M.sState;
	return temp;
}

/* Event loader */
void loaderReset(){
	midiInit();
}
unsigned char loaderDone(){
	return M.rCur>=MIDI_SIZE;
}
void load( unsigned short d ){
	if( M.sState ){         // state 1
		M.d[M.rCur].t=d;
		M.rCur++;
	}
	else{                   // state 0
		M.d[M.rCur].p1=(unsigned char)(d>>8);
		M.d[M.rCur].p2=(unsigned char)(d&0xFF);
	}
	M.sState=!M.sState;
}
void loaderFinalize(){
	midiInit();
	M.rTop=M.rCur;
	M.rBot=M.rTop;
	M.eBot=M.rTop;
	M.eCur=M.rTop;
}

/*===============Below: unused USART functions======================================*/

////////////////////////////////////////////////////////////////////////////////
//Functionality - checks if USART is ready to send
//Parameter: usartNum specifies which USART is checked
//Returns: 1 if true else 0
unsigned char USART_IsSendReady(unsigned char usartNum)
{
	return (usartNum != 1) ? (UCSR0A & (1 << UDRE0)) : (UCSR1A & (1 << UDRE1));
}
////////////////////////////////////////////////////////////////////////////////
//Functionality - checks if USART has successfully transmitted data
//Parameter: usartNum specifies which USART is being checked
//Returns: 1 if true else 0
unsigned char USART_HasTransmitted(unsigned char usartNum)
{
	return (usartNum != 1) ? (UCSR0A & (1 << TXC0)) : (UCSR1A & (1 << TXC1));
}
////////////////////////////////////////////////////////////////////////////////
// **** WARNING: THIS FUNCTION BLOCKS MULTI-TASKING; USE WITH CAUTION!!! ****
//Functionality - checks if USART has recieved data
//Parameter: usartNum specifies which USART is checked
//Returns: 1 if true else 0
unsigned char USART_HasReceived(unsigned char usartNum)
{
	return (usartNum != 1) ? (UCSR0A & (1 << RXC0)) : (UCSR1A & (1 << RXC1));
}
////////////////////////////////////////////////////////////////////////////////
//Functionality - Flushes the data register
//Parameter: usartNum specifies which USART is flushed
//Returns: None
void USART_Flush(unsigned char usartNum)
{
	static unsigned char dummy;
	if (usartNum != 1) {
		while ( UCSR0A & (1 << RXC0) ) { dummy = UDR0; }
	}
	else {
		while ( UCSR1A & (1 << RXC1) ) { dummy = UDR1; }
	}
}
////////////////////////////////////////////////////////////////////////////////
// **** WARNING: THIS FUNCTION BLOCKS MULTI-TASKING; USE WITH CAUTION!!! ****
//Functionality - Sends an 8-bit char value
//Parameter: Takes a single unsigned char value
//			 usartNum specifies which USART will send the char
//Returns: None
void USART_Send(unsigned char sendMe, unsigned char usartNum)
{
	if (usartNum != 1) {
		while( !(UCSR0A & (1 << UDRE0)) );
		UDR0 = sendMe;
	}
	else {
		while( !(UCSR1A & (1 << UDRE1)) );
		UDR1 = sendMe;
	}
}
////////////////////////////////////////////////////////////////////////////////
// **** WARNING: THIS FUNCTION BLOCKS MULTI-TASKING; USE WITH CAUTION!!! ****
//Functionality - receives an 8-bit char value
//Parameter: usartNum specifies which USART is waiting to receive data
//Returns: Unsigned char data from the receive buffer
unsigned char USART_Receive(unsigned char usartNum)
{
	if (usartNum != 1) {
		while ( !(UCSR0A & (1 << RXC0)) ); // Wait for data to be received
		return UDR0; // Get and return received data from buffer
	}
	else {
		while ( !(UCSR1A & (1 << RXC1)) );
		return UDR1;
	}
}