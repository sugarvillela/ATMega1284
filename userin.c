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
#include "userin.h"
#include "buttonbus.h"
#include "menu.h"
#include "song.h"
#include "lcddual.h"
#include "UT.h"

/*==========================User in: character entry==========================*/

#define CONTENT_LEN 9
#define BLEFT		0
#define BINC		1
#define BDEC		2
#define BRIGHT		3

uchar lastUp;
uchar lastDn;

typedef struct {
	char label[9];
	char content[CONTENT_LEN];
	char curr;
	uchar i;
	void (*saveFunct)();

} uinStruct;
uinStruct uin;

void dispUin(){
	uin.content[uin.i]=uin.curr;
	uin.content[uin.i+1]='\0';
	LCD_Disp( 9, (const uchar*)uin.content, 1 );
}
void uinBack(){
	LCD_ClearScreen( 1 );
	LCD_Disp( 1, (const uchar*)uin.label, 1 );
	uin.content[uin.i]='\0';
	uin.i--;
	if(uin.i==(uchar)-1){
		uin.i=0;
	}
	dispUin();
}
void uinInc(){//scroll ascii 0-9, A-Z
	uin.curr++;
	if( uin.curr>'Z' ){
		uin.curr='0';
	}
	else if( uin.curr<'A' && uin.curr>'9' ){
		uin.curr='A';
	}
	dispUin();
	lastDn=0;
}
void uinDec(){
	uin.curr--;
	if( uin.curr<'0' ){
		uin.curr='Z';
	}
	else if( uin.curr<'A' && uin.curr>'9' ){
		uin.curr='9';
	}
	dispUin();
	lastUp=0;
}
void uinIncFast(){//scroll ascii 0-9, A-Z
	uchar inc=(uchar)(bEffHoldTime( BINC ) )-lastUp;
	if( (char)inc<0 ){ return; }
	uin.curr+=inc;
	if( uin.curr>'Z' ){
		uin.curr='0';
	}
	else if( uin.curr<'A' && uin.curr>'9' ){
		uin.curr='A';
	}
	dispUin();
	lastUp=inc;
	lastDn=0;
}
void uinDecFast(){
	uchar dec=(uchar)(bEffHoldTime( BDEC ) )-lastDn;
	if( (char)dec<0 ){ return; }
	uin.curr-=dec;
	if( uin.curr<'0' ){
		uin.curr='Z';
	}
	else if( uin.curr<'A' && uin.curr>'9' ){
		uin.curr='9';
	}
	dispUin();
	lastDn=dec;
	lastUp=0;
}
void uinAdvance(){
	if( uin.i< CONTENT_LEN-1 ){
		uin.i++;
		dispUin();
	}
}
void uinSave(){
	//if( uin.i>1 ){
		//uin.i--;
		//uin.content[uin.i]='\0';
	//}
	uin.content[uin.i]='\0';
	unbindAll();
	song_setTitle( uin.content );
	song_serialize();
	if( uin.saveFunct ){
		uin.saveFunct();
	}
}
void uinSetLabel( char label[] ){
	cp( uin.label, label );
	//uinOff();
}
void uinOn(){
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
	//LCD_Disp( 1, (const uchar*)"Quit Up Dwn NextHoldNext=Save", 0 );
	LCD_Disp( 1, (const uchar*)uin.label, 1 );
	bindPressEvent( BLEFT,	&uinBack	);	//move cursor left
	bindPressEvent( BINC,	&uinInc		);	//A to B to C etc
	bindPressEvent( BDEC,	&uinDec		);	//C to B to A etc
	bindPressEvent( BRIGHT, &uinAdvance );	//add a character
	bindHoldEvent(	BLEFT,	&uinOff		);	//back to menu
	bindHoldEvent(	BINC,	&uinIncFast );
	bindHoldEvent(	BDEC,	&uinDecFast );
	bindHoldEvent(	BRIGHT,	&uinSave	);
	lastUp=0;
	lastDn=0;
	/* Reset display in case this is not the first visit */
	uin.curr='A';
	uin.i=0;
	uin.content[0]='\0';
	dispUin();
}
void uinOff(){
	menuOn();
}
void uinGetVal( char buf[] ){//17 characters
	cp( buf, uin.content );
}

/*==========================User in: numeric entry============================*/

typedef struct {
	char label[9];
	uchar val;
	uchar lobound;
	uchar hibound;
	void (*saveFunct)();
	
} numinStruct;
numinStruct numin;

void dispNumin(){
	uchar buf[4];
	nToChars_fixed( 3, numin.val, buf );
	LCD_Disp( 9, (const uchar*)buf, 1 );
}
void numinInc(){
	if( numin.val<numin.hibound ){
		numin.val++;
		dispNumin();
	}
	lastDn=0;
}
void numinDec(){
	if( numin.val>numin.lobound ){
		numin.val--;
		dispNumin();
	}
	lastUp=0;
}
void numinIncFast(){
	uchar inc=(uchar)(bEffHoldTime( BINC ) )-lastUp;
	if( (char)inc<0 ){ return; }
	uin.curr+=inc;
	if( numin.val>numin.hibound ){
		numin.val=numin.hibound;
	}
	dispNumin();
	lastUp=inc;
	lastDn=0;
}
void numinDecFast(){
	uchar dec=(uchar)(bEffHoldTime( BDEC ) )-lastDn;
	if( (char)dec<0 ){ return; }
	numin.val-=dec;
	if( numin.val<numin.lobound ){
		numin.val=numin.lobound;
	}
	dispNumin();
	lastDn=dec;
	lastUp=0;
}
void numinSave(){
	if( numin.saveFunct ){
		numin.saveFunct();
	}
}
void numinSetLabel( char label[] ){
	cp( numin.label, label );
}
void numinSetDef( uchar setVal, uchar setBound ){
	numin.val=setVal;
	numin.hibound=setBound;
}
void numinOn(){
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
	//LCD_Disp( 1, (const uchar*)"Quit Up Dwn Save", 0 );
	LCD_Disp( 1, (const uchar*)numin.label, 1 );
	dispNumin();

	bindPressEvent( BLEFT,	&numinOff );
	bindPressEvent( BINC,	&numinInc );
	bindPressEvent( BDEC,	&numinDec );
	bindPressEvent( BRIGHT, &numinSave );
	bindHoldEvent( BINC,	&numinIncFast );
	bindHoldEvent( BDEC,	&numinDecFast );
	lastUp=0;
	lastDn=0;
}
void numinOff(){
	menuOn();
}
unsigned char numinGetVal(){
	return numin.val;
}
/*==========================User in: tap or clap==============================*/

#define TPS         1000    //number of ticks per second at 10ms period
#define TAP_PERIOD	10
#define TAP_C       60000L   //constant = 60*100 = ticks per minute

enum tap_states{ ON, OFF };

typedef struct {
	uchar enable;
	ushort raw;
	uchar val;
	uchar lobound;
	uchar hibound;
	uchar state;
	void (*saveFunct)();
} tapStruct;
tapStruct tap;

void writeTap(){
	uchar buf[4];
	nToChars_fixed( 3, tap.val, buf );
	LCD_Disp( 9, (const uchar*)buf, 1 );
}
void tapEvent(){
	if( tap.raw ){
		tap.val=( tap.val )? ( tap.val+(TAP_C/tap.raw) )/2 : (TAP_C/tap.raw);
		writeTap();
	}
	else{//first call
		tap.state=ON;
	}
	tap.raw=0;
}
void tapSave(){
	if( tap.saveFunct ){
		tap.saveFunct();
	}
}
void tapOn(){
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
	LCD_Disp( 1, (const uchar*)"Quit Tap Tap Save", 0 );
	LCD_Disp( 1, (const uchar*)"Tempo", 1 );
	/* tap defaults */
	tap.enable=0;
	tap.raw=0;
	tap.val=0;
	tap.state=OFF;
	/* Buttons */
	bindPressEvent( BLEFT,	&tapOff );
	bindPressEvent( BINC,	&tapEvent );
	bindPressEvent( BDEC,	&tapEvent );
	bindPressEvent( BRIGHT, &tapSave );
}
void tapOff(){
	menuOn();
}
unsigned char tapGetVal(){
	return tap.val;
}

void tap_tick(){//a stopwatch
	if( tap.state ){
		tap.raw+=TAP_PERIOD;
	}
}

/*==========================Shared Initializer================================*/

void uinInit(){
	lastUp=0;
	lastDn=0;
	/* numin defaults */
	cp( numin.label, "Tempo" );
	numin.val=120;
	numin.lobound=30;
	numin.hibound=240;
	/* uin defaults */
	cp( uin.label, "Title" );
	/* tap defaults */
	tap.enable=0;
	tap.raw=0;
	tap.val=0;
	tap.lobound=30;
	tap.hibound=240;
	tap.state=OFF;
}

/*==========================Logistics=========================================*/

void uinBindSaveEvent( void (*f)() ){
	uin.saveFunct=f;
}
void numinBindSaveEvent( void (*f)() ){
	numin.saveFunct=f;
}
void tapBindSaveEvent( void (*f)() ){
	tap.saveFunct=f;
}
void uinUnbindAll(){
	uin.saveFunct=0;
	numin.saveFunct=0;
	tap.saveFunct=0;
}


