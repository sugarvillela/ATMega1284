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

#ifndef READWRITE_H_
#define READWRITE_H_

unsigned char writeChar( unsigned char addr, unsigned char writeMe );
unsigned char readChar( unsigned char addr );
unsigned char writeShort( unsigned short addr, unsigned short writeMe );
unsigned short readShort( unsigned short addr );

/*=============Preferences====================================================*/

/* store/get preferences by number */
#define MET_PLAY    1   //Metronome on playback
#define MET_REC     2   //Metronome on record
#define DISP_MB     4   //M:B or raw time

unsigned char setPref( unsigned char which, unsigned char what );
unsigned char getPref( unsigned char which );
unsigned char wipePref();

/*=============Header=========================================================*/

unsigned char fWriteHeader( char title[], unsigned char tempo );
unsigned char fWriteSize( unsigned short size );

void fReadTitle( char buf[] );
unsigned char fReadTempo();
unsigned short fReadSize();
unsigned char isFile();

/*=============Data Dumper====================================================*/

/* serializer */

void fPutReset();
unsigned char fPut( unsigned short d );
unsigned char fPutFinalize();

/* unserializer */

void fGetReset();
unsigned char fGetDone();
unsigned short fGet();

unsigned short fGetSize();
//void serTest();

#endif /* READWRITE_H_ */