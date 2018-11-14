#include <avr/io.h>
#include "userin.h"
#include "buttonbus.h"
#include "menu.h"
#include "lcddual.h"
#include "UT.h"

/*==========================User in: character entry==========================*/

#define CONTENT_LEN 17
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

} uinStruct;
uinStruct uin;

void writeUin(){
	uin.content[uin.i]=uin.curr;
	uin.content[uin.i+1]='\0';
	LCD_Disp( 9, (const uchar*)uin.content, 1 );
	//printf("%-16s %-8s\n", uin.label, uin.content );
}
void uinBack(){
	if( uin.i>1 ){
		LCD_ClearScreen( 1 );
		LCD_Disp( 1, (const uchar*)uin.label, 1 );
		uin.content[uin.i]='\0';
		uin.i--;
		writeUin();
	}
}
void uinInc(){//scroll ascii 0-9, A-Z
	uin.curr++;
	if( uin.curr>'Z' ){
		uin.curr='0';
	}
	else if( uin.curr<'A' && uin.curr>'9' ){
		uin.curr='A';
	}
	writeUin();
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
	writeUin();
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
	writeUin();
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
	writeUin();
	lastDn=dec;
	lastUp=0;
}
void uinAdvance(){
	if( uin.i< CONTENT_LEN-1 ){
		uin.i++;
		writeUin();
	}
}
void uinSave(){
	if( uin.i>1 ){
		uin.i--;
		uin.content[uin.i]='\0';
		writeUin();
	}
	uinOff();
}

void uinSetLabel( char label[] ){
	cp( uin.label, label );
	uinOff();
}
void uinOn(){
	//printf( "Quit Up Dwn Save\n");
	LCD_ClearScreen( 0 );
	LCD_ClearScreen( 1 );
	//LCD_Disp( 1, (const uchar*)"Quit Up Dwn NextHoldNext=Save", 0 );
	LCD_Disp( 1, (const uchar*)uin.label, 1 );
	bindPressEvent( BLEFT,	&uinBack );
	bindPressEvent( BINC,	&uinInc );
	bindPressEvent( BDEC,	&uinDec );
	bindPressEvent( BRIGHT, &uinAdvance );
	bindHoldEvent( BLEFT,	&uinOff );
	bindHoldEvent( BINC,	&uinIncFast );
	bindHoldEvent( BDEC,	&uinDecFast );
	bindHoldEvent( BRIGHT,	&uinSave );
	lastUp=0;
	lastDn=0;
	writeUin();
}
void uinOff(){
	//LCD_Disp( 1, (const uchar*)"uinOff", 1 );
	//LCD_ClearScreen();
	menuOn();
}

/*==========================User in: numeric entry============================*/

typedef struct {
	char label[9];
	uchar val;
	uchar lobound;
	uchar hibound;
	
} numinStruct;
numinStruct numin;

void writeNumin(){
	uchar buf[4];
	nToChars_fixed( 3, numin.val, buf );
	LCD_Disp( 9, (const uchar*)buf, 1 );
	//printf("%-8s %-8s\n", numin.label, buf );
}

void numinInc(){
	if( numin.val<numin.hibound ){
		numin.val++;
		writeNumin();
	}
	lastDn=0;
}
void numinDec(){
	if( numin.val>numin.lobound ){
		numin.val--;
		writeNumin();
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
	writeNumin();
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
	writeNumin();
	lastDn=dec;
	lastUp=0;
}

void numinSave(){
	//printf( "Num in Save\n");
	numinOff();
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
	writeNumin();

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
	//LCD_Disp( 1, (const uchar*)"numinOff", 1 );
	//LCD_ClearScreen();
	menuOn();
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
		//printf("subsequent: ticks=%d, tempo=%d\n", tap.raw, tap.val );
		writeTap();
	}
	else{//first call
		//printf("initial: %d\n", tap.raw );
		tap.state=ON;
	}
	tap.raw=0;
}
void tapSave(){
	//printf( "Num in Save\n");
	tapOff();
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
	//LCD_Disp( 1, (const uchar*)"tapOff", 1 );
	//LCD_ClearScreen();
	menuOn();
}

int tap_tick( int state ){//a stopwatch
	if( tap.state ){
		tap.raw+=TAP_PERIOD;
	}
	return 0;
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
	uin.curr='A';
	uin.i=0;
	/* tap defaults */
	tap.enable=0;
	tap.raw=0;
	tap.val=0;
	tap.lobound=30;
	tap.hibound=240;
	tap.state=OFF;

	//test
	//uchar i;
	//for( i=0; i< 200; i++){
		//tap_tick( 0 );
		//if( i && i%6==0 ){
			//tapEvent();
		//}
		////printf( "%c\n", uin.curr );
	//}
}


