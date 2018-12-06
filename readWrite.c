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

#include <avr/eeprom.h>
#include "readWrite.h"
#include "lcddual.h"
#include "UT.h"

/* Serial constants */

#define ADDR_PREF   8
#define ADDR_HEAD   16
#define ADDR_VAL	27
#define ADDR_SIZE	28
#define SER_BASE    32

unsigned char writeChar( uchar addr, uchar writeMe ){
	eeprom_write_byte ( (uint8_t*)addr, (uint8_t)writeMe );
	uint8_t checkMe=eeprom_read_byte ( (uint8_t*)addr );
	return checkMe==writeMe;
}
unsigned char readChar( uchar addr ){
	return (unsigned char)eeprom_read_byte ( (uint8_t*)addr );
}
unsigned char writeShort( ushort addr, ushort writeMe ){
	eeprom_write_word( (uint16_t*)addr, (uint16_t)writeMe );
	uint16_t checkMe=eeprom_read_word( (const uint16_t*)addr );
	return checkMe==writeMe;
}
unsigned short readShort( ushort addr ){
	return (unsigned short)eeprom_read_word( (const uint16_t*)addr );
}

/*=============Preferences====================================================*/

#define PREF_DEF    6       //defaults
#define BIT_CHECK   160     //a way of telling if preferences have been written

/* store/get preferences by number */
unsigned char setPref( unsigned char which, unsigned char what ){
	unsigned char curr=readChar( ADDR_PREF );
	if( (curr&BIT_CHECK) == BIT_CHECK ){
		if( what ){
			curr|=which;
		}
		else{
			curr&=(unsigned char)~which;
		}
		return writeChar( ADDR_PREF, curr );;
	}
	return 0;//err state
}
unsigned char getPref( unsigned char which ){
	unsigned char curr=readChar( ADDR_PREF );
	if( (curr&BIT_CHECK)!= BIT_CHECK ){
		//printf("bad bit check: %X\n", (curr&BIT_CHECK) );
		writeChar( ADDR_PREF, PREF_DEF|BIT_CHECK );//set default
		curr=PREF_DEF;
		//printf("bad bit check: curr=%X\n", curr );
	}
	//printf("which=%X\n", which );
	return (curr&which)? 1:0;
}
unsigned char wipePref(){
	return writeChar( ADDR_PREF, PREF_DEF|BIT_CHECK );
}

/*=============File Header====================================================*/
/* Write */
unsigned char fWriteHeader( char title[], unsigned char tempo ){
	/* Header is 10 bytes: 9 for title and 1 for tempo */
	uchar i;
	for(i=0; i<9; i++ ){
		if( !writeChar( ADDR_HEAD+i, (uchar)title[i] ) ){
			return 0;
		}
		if( !title[i] ){
			break;
		}
	}
	return writeChar( ADDR_HEAD+9, tempo ) && writeChar( ADDR_VAL, BIT_CHECK );//
}
unsigned char fWriteSize( unsigned short size ){
	return writeShort( ADDR_SIZE, size );
}
void fReadTitle( char buf[] ){
	uchar i;
	for( i=0; i<9; i++ ){
		buf[i]=(char)readChar( ADDR_HEAD+i );
		if( !buf[i] ){
			return;
		}
	}
}
unsigned char fReadTempo(){
	return readChar( ADDR_HEAD+9 );
}
unsigned short fReadSize(){
	return readShort( ADDR_SIZE );
}

unsigned char isFile(){
	uchar val=readChar( ADDR_VAL );
	return ( val&BIT_CHECK )==BIT_CHECK;
}
/*=============Data Dumper====================================================*/

//unsigned short ser[35];//dummy

/* serializer */
unsigned short seri, serSize;

void fPutReset(){
	seri=1;
}
unsigned char fPut( unsigned short d ){
	if( !writeShort( seri+SER_BASE, d ) ){
		return 0;
	}
	seri++;
	return 1;
}
unsigned char fPutFinalize(){
	unsigned short temp=seri+1;//file size with 16-bit header
	seri=0;
	return fPut( temp );//write the header at 0
}

/* unserializer */
void fGetReset(){
	seri=0;
	serSize=fGet();//gets size and increments seri to 1;
}
unsigned short fGet(){
	unsigned short d=readShort( SER_BASE+seri );
	seri++;
	return d;
}
unsigned char fGetDone(){
	return seri>=serSize;
}

unsigned short fGetSize(){
	return ( isFile() )? serSize : 0;
}

