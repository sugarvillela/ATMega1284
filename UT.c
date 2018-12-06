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

#include "UT.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "lcddual.h"

/*	Abstracts the IO settings; Specify in/out as 4 8-char strings of i or o  */
void setIO( char ioa[], char iob[], char ioc[], char iod[]){
	uchar ddra=0, ddrb=0, ddrc=0, ddrd=0, mask, i;
	for( i=0, mask=0x80; i<8; mask=mask>>1,i++ ){
		if(ioa[i]=='o'){ ddra|=mask; }
		if(iob[i]=='o'){ ddrb|=mask; }
		if(ioc[i]=='o'){ ddrc|=mask; }
		if(iod[i]=='o'){ ddrd|=mask; }
	}
	DDRA=ddra; PORTA=(uchar)~ddra;
	DDRB=ddrb; PORTB=(uchar)~ddrb;
	DDRC=ddrc; PORTC=(uchar)~ddrc;
	DDRD=ddrd; PORTD=(uchar)~ddrd;
}
/*	Enter binary as 8-element char array like 10100101 */
uchar toBin( char bin[], char one ){
	uchar out=0;
	uchar mask, i;
	for( i=0, mask=0x80; i<8; mask=mask>>1,i++ ){
		if(bin[i]==one ){ out|=mask; }
	}
	return out;
}
void toBinArr( char* binIn[], uchar binOut[], uchar size, char one ){
	uchar i;
	for( i=0; i<size; i++ ){
		binOut[i]=toBin( binIn[i], one );
	}
}
/* Returns abs value */
unsigned char ab( char in ){//absolute value
	return in>=0? in : (~in)+1;
}
/* Counts char length until '\0'.  Bad string will break it */
unsigned char len( const char str[] ){
	unsigned char i=0;
	for ( i=0; i< 0xFF; i++ ){
		if( str[i]=='\0' ){
			return i;
		}
	}
	return i;
}
/* returns a==b */
unsigned char sameString( char a[], char b[] ){
	unsigned char i=0;
	while( a[i] ){
		if( a[i]!=b[i] ){ return 0; }//catches b shorter
		i++;
	}
	return b[i]=='\0';//catches b longer
}

void cp( char to[], char from[] ){
	uchar i=0;
	while( from[i] ){
		to[i]=from[i];
		i++;
	}
	to[i]='\0';
}
void cpto( uchar offset, char to[], char from[] ){
	uchar i=0;
	while( from[i] ){
		to[i+offset]=from[i];
		i++;
	}
	to[i+offset]='\0';
}

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
/* Finds log 2 of input number */
unsigned char binPlaces( unsigned short in ){
	if( in==0 ){ return 1; }
	unsigned char i;
	for ( i=0; in && i< 0xFF; i++ ){
		in=in>>1;
	}
	return i;
}
/* For LCD display, returns input number in string form; buf size must be sufficient */
void nToChars( unsigned short in, unsigned char buf[] ){//buffer 6 for 16 bit range
	unsigned char n = decPlaces( in );
	unsigned short next;
	short i;
	for ( i=n-1; i>=0; i-- ){
		next = in/10;
		buf[i]=(in - ( next*10 ))+'0';
		in = next;
	}
	buf[n]='\0';
}
void nToHex( unsigned short in, unsigned char buf[] ){//buffer 5 for 16 bit range
	unsigned char n = hexPlaces( in );
	char hex[]="0123456789ABCDEF";
	int i;
	//printf("nToHex: %d\n", in );
	for ( i=n-1; i>=0; i-- ){
		buf[i]=hex[in&0x0F];
		//printf("i=%d, n=%d\n", i, in );
		in=in>>4;
	}
	buf[n]='\0';
}
void nToBin( unsigned short in, unsigned char buf[] ){//buffer 17 for 16 bit range
	unsigned char n = binPlaces( in );
	int i;
	for ( i=n-1; i>=0; i-- ){
		buf[i]=(in&0x01)+'0';
		in=in>>1;
	}
	buf[n]='\0';
}
void fToChars( float fin, unsigned char buf[] ){
/*	Returns float number as string to two or four decimal places; 
	For range > 655, make 'in' a long int. (656*100 overruns short)
	Currently, buf must be length 9: log_10(2^16) + dot + 2 decimal + null 
	Using int, buf would be length 13: log_10(2^32) + dot + 2 decimal + null */
	uchar neg, len, point;
	ushort in, next;
	/* Fix negative values */
	if( fin<0 ){
		fin*=-1;
		buf[0]='-';
		neg=1;
	}
	else{
		neg=0;
	}
	/* set precision */
	if( fin<(float)6.55){
		in=(ushort)( fin*10000 );
		point=4;
	}
	else{
		in=(ushort)( fin*100 );
		point=2;
	}
	len = neg+decPlaces( in );
	int i;
	for ( i=len; i>=neg; i-- ){
		if( i==len-point ){
			buf[i]='.';
		}
		else{
			next = in/10;
			buf[i]=( in - ( next*10 ) ) +'0';
			in = next;
		}
	}
	buf[len+1]='\0';
}

/* If you know your number is 0-9, simplify */
void nToChars_digit( uchar in, uchar buf[] ){
	buf[0]=in+'0';
	buf[1]='\0';
}
/* If always same power of 10, simplify */
void nToChars_fixed( uchar width, uchar in, uchar buf[] ){
	uchar next;
	short i;
	for ( i=width-1; i>=0; i-- ){
		next = in/10;
		buf[i]=(in - ( next*10 ))+'0';
		in = next;
	}
	i=0;
	while(buf[i]=='0'){
		buf[i]=' ';
		i++;
	}
	buf[width]='\0';
}

/* Human-readable */
void onOff( uchar onOff, uchar buf[] ){
	if( onOff )	{ buf[0]='O'; buf[1]='n'; buf[2]='\0'; }
	else		{ buf[0]='O'; buf[1]='f'; buf[2]='f'; buf[3]='\0'; }
}

void progress( uchar state, ushort n ){
	/*	This is sort of an easter egg...
		state 0: start progress bar; pass loop length via 'n' parameter
		state 1: display progress bar; pass current 'i' via 'n' parameter
		state 2: end progress bar; clean up
	*/
    static char bar[9];
    static uchar i, segment;
    static ushort inc;
    switch( state ){
        case 0://start
            cli();//stop timer and USART interrupts
            LCD_ClearScreen( 1 );
            inc=n/8;
            segment=2;
			LCD_Disp( 1, (const uchar*)"Working ", 1 );
			delay_ms( 200 );//guarantee minimally visible speed
            return;
        case 1://progress
            if( n<inc ){
                return;
            }
            for( i=0;i<segment && i<9;i++){
                bar[i]='=';
            }
            bar[i]='\0';
			LCD_Disp( 9, (const uchar*)bar, 1 );
            segment+=2;
            inc+=inc;
			delay_ms( 200 );
            return;
        default://finish
            segment=4;
            LCD_ClearScreen( 1 );
            sei();//restart timer and USART interrupts
    }
}
/* Code as given */
/* From io.c */
void delay_ms(int miliSec){ //for 8 Mhz crystal
	int i, j;
	for(i=0;i<miliSec;i++){
		for(j=0;j<775;j++){
			asm("nop");
		}
	}
}

unsigned long int findGCD(unsigned long int a, unsigned long int b){
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
/* PWM functions */
void set_PWM(double frequency) {
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}
void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}
void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//unsigned char SetBit(unsigned char pin, unsigned char number, unsigned char bin_value){
	//return (bin_value ? pin | (0x01 << number) : pin & ~(0x01 << number));
//}
//unsigned char GetBit(unsigned char port, unsigned char number){
	//return ( port & (0x01 << number) );
//}

//unsigned char GetKeypadKey() {
//PORTA = 0xEF; // Enable col 4 with 0, disable others with 1’s
//asm("nop"); // add a delay to allow PORTC to stabilize before checking
//if (GetBit(PINA,0)==0) { return('1'); }
//if (GetBit(PINA,1)==0) { return('4'); }
//if (GetBit(PINA,2)==0) { return('7'); }
//if (GetBit(PINA,3)==0) { return('*'); }
//
//Check keys in col 2
//PORTA = 0xDF; // Enable col 5 with 0, disable others with 1’s
//asm("nop"); // add a delay to allow PORTC to stabilize before checking
//if (GetBit(PINA,0)==0) { return('2'); }
//if (GetBit(PINA,1)==0) { return('5'); }
//if (GetBit(PINA,2)==0) { return('8'); }
//if (GetBit(PINA,3)==0) { return('0'); }
//
//Check keys in col 3
//PORTA = 0xBF; // Enable col 6 with 0, disable others with 1’s
//asm("nop"); // add a delay to allow PORTC to stabilize before checking
//if (GetBit(PINA,0)==0) { return('3'); }
//if (GetBit(PINA,1)==0) { return('6'); }
//if (GetBit(PINA,2)==0) { return('9'); }
//if (GetBit(PINA,3)==0) { return('#'); }
//
//Check keys in col 4
//PORTA = 0x7F; // Enable col 6 with 0, disable others with 1’s
//asm("nop"); // add a delay to allow PORTC to stabilize before checking
//if (GetBit(PINA,0)==0) { return('A'); }
//if (GetBit(PINA,1)==0) { return('B'); }
//if (GetBit(PINA,2)==0) { return('C'); }
//if (GetBit(PINA,3)==0) { return('D'); }
//return('\0'); // default value
//}