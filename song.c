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
#include "song.h"
#include "lcddual.h"
#include "midiin.h"
#include "readWrite.h"
#include "transport.h"
#include "menu.h"
#include "UT.h"

typedef struct {
	char title[TITLE_LEN];
	uchar isTitled;			//false while title equals 'UNTITLED'
} songStruct;
songStruct song;
/* Accessors */
void song_getTitle( char buf[] ){
	if( song.title ){
		cp( buf, song.title );
	}
}
unsigned char song_isTitled(){
	return song.isTitled;
}	
	
/* Mutators */
void song_setTitle( char buf[] ){
	uchar i=0;
	while( buf[i] && i<TITLE_LEN){
		song.title[i]=buf[i];
		i++;
	}
	song.title[i]='\0';
	song.isTitled=1;
}
void song_serialize(){
	fWriteHeader( song.title, getTempo() );//song.title
	midiToFile();
}
void song_load(){
	//if(!isFile()){
		//msgBindDropEvent( 1, &menuOn );
		//msgWriteErr( 1, "No File", MSG_SHORT );
		//return;
	//}
	/* header */
	fReadTitle( song.title );
	setTempo( fReadTempo() );
	song.isTitled=1;
	/* dump from file */
	midiFromFile();
}
void song_init(){
	cp( song.title, "UNTITLED" );
	song.isTitled=0;
}