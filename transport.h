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

#ifndef METRONOME_H_
#define METRONOME_H_

#define BEEP_PERIOD		5
#define MET_PERIOD		50		//10 milliseconds


enum tsigs{ THREE_FOUR, FOUR_FOUR, SIX_EIGHT };

/* Mutators */
void transportInit();
void transportOn();
void play();
void record();
void transportStop();
void transportTop();
void transportRW();
void transportFF();

void startupDisplay();

void setTempo( unsigned char tempo );
void setTimeSignature( int tsig );

/* Sound settings (light always flashes) */
void toggleBeepOn();
void toggleDispMS();

/* Accessors */
void getTimeSig_str( unsigned char buf[] );		// 4:4, 3:4 or 6:8 as string
void getTempo_str( unsigned char buf[] );		// nToChar current tempo
void getTimeMB( unsigned char buf[] );			// 1:1 Current metronome

unsigned char getTempo();
unsigned char getTsig1();						// 4:4 returns first number
unsigned char getTsig2();						// 4:4 returns second number
unsigned char getCurrBeat();
unsigned char getCurrMeasure();
unsigned char getBeepOn();
unsigned char getDispMSOn();
float getBeatValue();

void metronome_tick();
void beep_tick();

#endif /* METRONOME_H_ */