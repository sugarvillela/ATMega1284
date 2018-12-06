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

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <avr/interrupt.h>

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long tasksPeriodGCD = 1; // Start count from here, down to 0. Default 1ms
unsigned long tasksPeriodCntDown = 0; // Current internal count of 1ms ticks

unsigned char tasksNum = 0; // Number of tasks in the scheduler. Default 0 tasks

////////////////////////////////////////////////////////////////////////////////
// Struct for Tasks represent a running process in our simple real-time operating system
typedef struct task {
	//signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	void (*TickFct)(); 		//Task tick function
} task; 

task* tasks;

///////////////////////////////////////////////////////////////////////////////
// Heart of the scheduler code
void TimerISR() {
    static unsigned char i;
    for (i = 0; i < tasksNum; i++) { 
        if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
            tasks[i].TickFct();
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += tasksPeriodGCD;
    }
}

///////////////////////////////////////////////////////////////////////////////
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT0 == OCR0 (every 1 ms per TimerOn settings)
	tasksPeriodCntDown--; 			// Count down to 0 rather than up to TOP
	if (tasksPeriodCntDown == 0) { 	// results in a more efficient compare
		TimerISR(); 				// Call the ISR that the user uses
		tasksPeriodCntDown = tasksPeriodGCD;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Set TimerISR() to tick every m ms
void TimerSet(unsigned long m) {
	tasksPeriodGCD = m;
	tasksPeriodCntDown = tasksPeriodGCD;
}

///////////////////////////////////////////////////////////////////////////////
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B 	= (1<<WGM12)|(1<<CS11)|(1<<CS10);
                    // WGM12 (bit3) = 1: CTC mode (clear timer on compare)
					// CS12,CS11,CS10 (bit2bit1bit0) = 011: prescaler /64
					// Thus TCCR1B = 00001011 or 0x0B
					// So, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
					// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
/* Notes on this:
	Midi baud rate is 31250 bits per second, 3906 bytes/sec
	seconds or 256 microseconds (the minimum period)
	Setting OCR1A to 25 gives period of 200 microseconds by calculation
	In real life, 24 is closer.
*/
	OCR1A 	= 25;	// Timer interrupt will be generated when TCNT1==OCR1A
					// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
					// So when TCNT1 register equals 125,
					// 1 ms has passed. Thus, we compare to 125.
					// AVR timer interrupt mask register

#if defined (__AVR_ATmega1284__)
    TIMSK1 	= (1<<OCIE1A); // OCIE1A (bit1): enables compare match interrupt - ATMega1284
#else
    //TIMSK 	= (1<<OCIE1A); // OCIE1A (bit1): enables compare match interrupt - ATMega32
#endif

	// Initialize avr counter
	TCNT1 = 0;

	// TimerISR will be called every tasksPeriodCntDown milliseconds
	tasksPeriodCntDown = tasksPeriodGCD;

	// Enable global interrupts
	SREG |= 0x80;	// 0x80: 1000000
}

#endif //SCHEDULER_H
