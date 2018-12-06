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

#ifndef UT_H
#define UT_H

#define uchar unsigned char
#define uint unsigned int
#define ushort unsigned short
#define ulong unsigned long

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))	//returns a power of two number
#define BIT(p,i) (((p)>>(i))&1)			//returns 0 or 1

#define SET_TWO(p,i) ((p) |= (3 << (i)))
#define CLR_TWO(p,i) ((p) &= ~(3 << (i)))

enum { BYP=100 };//for turning off state machines
enum numForms{ NUM_DEC, NUM_BIN, NUM_HEX };//for msgWrite_num
	
/*	Abstracts IO settings; Specify in/out as 4 8-char strings of i or o, like "iiiioooo" */
void setIO( char ioa[], char iob[], char ioc[], char iod[]);

/*	Enter binary as 8-element char array like 10100101 */
uchar toBin( char bin[], char one );
void toBinArr( char* binIn[], uchar binOut[], uchar size, char one );

unsigned char ab( char in );//absolute value
unsigned char len( const char str[] );
unsigned char sameString( char a[], char b[] );

/* copy functions to leave c libraries out */
void cp( char to[], char from[] );
void cpto( uchar offset, char to[], char from[] );

/* Finds log 10 of input number */
unsigned char decPlaces( unsigned short in );
/* Finds log 16 of input number */
unsigned char hexPlaces( unsigned short in );
/* Finds log 2 of input number */
unsigned char binPlaces( unsigned short in );

/* For LCD display, returns input number in string form; buf size must be sufficient */
void nToChars( ushort in, uchar buf[] );
void nToHex( ushort in, uchar buf[] );
void nToBin( ushort in, uchar buf[] );
void fToChars( float in, uchar buf[] );

/* If you know your number is 0-9, simplify */
void nToChars_digit( uchar in, uchar buf[] );
/* If always same power of 10, simplify */
void nToChars_fixed( uchar width, uchar in, uchar buf[] );
/* Human-readable bool to "On" or "Off" */
void onOff( uchar onOff, uchar buf[] );
/* Progress bar (might be too quick to display)*/
void progress( uchar state, ushort n );

/* Code as given */
void delay_ms(int miliSec);//from io.c

unsigned char SetBit(unsigned char pin, unsigned char number, unsigned char bin_value);
unsigned char GetBit(unsigned char port, unsigned char number);
unsigned long int findGCD(unsigned long int a, unsigned long int b);

/* PWM functions */
void set_PWM(double frequency);
void PWM_on();
void PWM_off();

#endif //UT_H