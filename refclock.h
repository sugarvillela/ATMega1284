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

#ifndef REFCLOCK_H_
#define REFCLOCK_H_

#define REFCLK_PERIOD 50 //10 ms

void refclockOn();
void refclockOff();
void zeroRefclock();
void refclockOverride( unsigned short setRaw );

unsigned short getTime();
void getTimeMSF( char buf[] );
void refclock_tick();

#endif /* REFCLOCK_H_ */