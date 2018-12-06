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

#ifndef LCDDUAL_H
#define LCDDUAL_H

#define	MSG_PERIOD	125	//	25 milliseconds

/* For timeout you can pass a number, or just use one of these constants */
#define MSG_SHORT	25	//	0.5 seconds
#define MSG_LONG	77	//	2 seconds
#define FOREVER		255

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long

/*===============Direct LCD write============================================*/

void LCD_init(void);
void LCD_ClearScreen( uchar wScreen );
void LCD_WriteCommand (uchar Command, uchar wScreen  );
void LCD_WriteData( uchar Data, uchar wScreen );
void LCD_Disp( uchar column, const uchar* string, uchar wScreen );//no clear screen
void LCD_DispNew( uchar column, const uchar* string, uchar wScreen );//with clear screen
void LCD_Cursor( uchar column,  uchar wScreen );

/*===============LCD using message queue======================================*/

void msgInit();

/* Set timeout using defines above or any number < 254 */
void msgWrite( uchar wScreen, char buf[], unsigned char setTimeout );
void msgWrite_num( uchar wScreen, int format, char label[], ushort num, uchar setTimeout );
void msgWriteErr( uchar wScreen, char buf[], uchar setTimeout );

uchar fullQ( uchar wScreen );
uchar emptyQ();
uchar numInQ( uchar wScreen );
uchar msgIsLocked( uchar wScreen );
void msgClear( uchar wScreen );
void dispWelcomeScreen();

void msgBindDropEvent( uchar wScreen, void (*f)() );
void msgUnbindDropEvent( uchar wScreen );
void msgUnbindAll();

/*	If queue is too fancy for ATMega, shut off queue
	and manage screens manually */
void msgQueueOn( uchar wScreen );
void msgQueueOff( uchar wScreen );

void message_tick();
void setScreen( uchar scr );

#endif