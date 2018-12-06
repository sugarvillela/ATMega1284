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
#include <avr/interrupt.h>
#include "UT.h"
#include "scheduler.h"	
#include "midiin.h"
#include "lcddual.h"
#include "shiftreg16.h"
#include "ledmatrix.h"
#include "buttonbus.h"
#include "transport.h"
#include "refclock.h"
#include "song.h"
#include "menu.h"
#include "userin.h"

#define NUM_TASKS	6

int main(void){//DDR 0 for input, F for output
	tasksNum = NUM_TASKS;//don't delete this!!!!
	setIO(
		"iiiiiiii",	//IO A	LSB on Right!
		"oooooooo",	//IO B
		"oooooooo",	//IO C
		"ooooiiii"	//IO D
	);
	ulong int GCD;

	task tsks[NUM_TASKS];
	tasks = tsks;
	
	uchar i = 0;
	/* reference clock */
	tasks[i].period = REFCLK_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &refclock_tick;
	i++;
	/* metronome */
	tasks[i].period = MET_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &metronome_tick;
	i++;
	/* metronome beep*/
	tasks[i].period = BEEP_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &beep_tick;
	i++;
	/* tap clock */
	tasks[i].period = REFCLK_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &tap_tick;
	i++;
	/* message_tick */
	tasks[i].period = MSG_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &message_tick;
	i++;
	/* Buttons */
	tasks[i].period = B_BUS_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonBus_tick;
	i++;
	
	GCD = tasks[0].period;
	for( i=1; i<NUM_TASKS; i++){
		GCD = findGCD( GCD, tasks[i].period );
	}
	bBusInit();//initialize this first
	transportInit();
	msgInit();
	uinInit();
	midiInit();
	song_init();
	menuInit();//initialize this last
	msgBindDropEvent( 0, &transportOn );
	dispWelcomeScreen();
	TimerSet( GCD );
	TimerOn();
	while(1) {}
}
