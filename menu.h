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

#ifndef MENU_H
#define MENU_H

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long 

/* 'public' functions */
typedef struct menuStruct mStruct;
struct menuStruct{
    char name[9];
    mStruct* left;
    mStruct* right;
    mStruct* up;
    mStruct* down;
    void (*value)( uchar buf[] );
    void (*go)();
};

void menuInit();
void menuOn();
void menuOff();
void menuWrite();

#endif //MENU_H