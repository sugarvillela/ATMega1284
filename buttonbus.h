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

#ifndef BUTTONBUS_H
#define BUTTONBUS_H

#define	B_BUS_PERIOD	31// 125	//5 milliseconds

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long

/* 'public' functions */
/* accessors */
uchar bPressed( uchar i );
uchar bWasPressed( uchar i );
uchar bHeld( uchar i );
ushort bHoldTime( uchar i );
ushort bEffHoldTime( uchar i );
uchar bPressCount( uchar i );
uchar bStateMap( uchar offset );
uchar bHoldMap( uchar offset );

/* Mutators */
void bBusInit();
void bReset( uchar i );

/*=================Function pointers as event listeners=======================*/

void bindPressEvent( uchar i, void (*f)() );
void bindHoldEvent( uchar i, void (*f)() );
void bindDoubleEvent( uchar i, void (*f)() );
void unbindAll();

void buttonBus_tick();

#endif //BUTTONBUS_H