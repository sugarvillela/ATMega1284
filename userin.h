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

#ifndef USERIN_H_
#define USERIN_H_

/*==========================User in: character entry==========================*/

void uinSetLabel( char label[] );
void uinOn();
void uinOff();
void uinGetVal( char buf[] );//17 characters

/*==========================User in: numeric entry============================*/

void numinSetLabel( char label[] );
void numinSetDef( unsigned char setVal, unsigned char setBound );
void numinOn();
void numinOff();
unsigned char numinGetVal();

/*==========================User in: tap tempo================================*/

void tapOn();
void tapOff();
void tap_tick();
unsigned char tapGetVal();

/*==========================Shared Initializer and Unbind=====================*/

void uinInit();
void uinUnbindAll();

/*==========================Logistics=========================================*/

void uinBindSaveEvent( void (*f)() );
void numinBindSaveEvent( void (*f)() );
void tapBindSaveEvent( void (*f)() );

#endif /* USERIN_H_ */