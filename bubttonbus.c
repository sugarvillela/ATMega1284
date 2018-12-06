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
#include "buttonbus.h"
#include "UT.h"
#include "lcddual.h"

/*  A tool for momentary buttons on a single bus A, B, C or D:
    Tick function is shared between all buttons, with the SM state
	held in each button struct. To set tick period, decide period 
	for one button, then divide by the number of buttons.
	
    Tick functions resets 'pressed' field.
    Conversely, 'held', 'wasPressed', 'holdTime', 'pressCount' fields
	must be reset by another part of the program (the one waiting 
	for an event calls reset upon receipt of the event).
	
    Buttons must be consecutively placed, but not necessarily 
	at 0 (use B_OFFSET). For example, this program uses pins 2-5,
	saving pin 1 for an A/D input I never used.  To use 1-4, set
	offset to 0
*/

#define N_BUTTONS       4       //number of buttons (see above)
#define INBUS           PINA    //input bus
#define B_OFFSET        1       //buttons don't have to start at pin 0

#define THRESH_ON		3		//12*5=60ms;  60 on 60 off = 120ms 
#define THRESH_OFF		3		
#define THRESH_HOLD		50		//250*5=1.25 seconds

enum bstates{ INIT, OFF, BOUNCEON, ON, BOUNCEOFF, RESET };
	
typedef struct {
    uchar bounceCount;			//tick counter for debouncing
    ushort holdCount;			//tick counter for comparing to THRESH_HOLD
    uchar pressed;				// resets on button release
	uchar wasPressed;			// persistent until checked, reset
	uchar held;					// persistent until checked, reset
	uchar pressCount;			// persistent until checked, reset
	void (*pFunct)();			// press event
	void (*hFunct)();			// hold event
	void (*dFunct)();			// double-click event
	
	int state;
} buttonStruct;

/*	
	Each button is an element on this array 
	'B' is the current index (modified each tick) 
*/
buttonStruct buttons[N_BUTTONS];
uchar B;

/* 'public' functions */
/* accessors */

uchar bPressed( uchar i ){
    return (i<N_BUTTONS)? buttons[i].pressed : 0;
}
uchar bWasPressed( uchar i ){
	return (i<N_BUTTONS)? buttons[i].wasPressed : 0;
}
uchar bHeld( uchar i ){
    return (i<N_BUTTONS)? buttons[i].held : 0;
}
ushort bHoldTime( uchar i ){
	return buttons[i].holdCount;
}
ushort bEffHoldTime( uchar i ){
	return ( buttons[i].holdCount > THRESH_HOLD )? buttons[i].holdCount-THRESH_HOLD : 0;
}
uchar bPressCount( uchar i ){
	return buttons[i].pressCount;
}

/* Mutators */
void bReset( uchar i ){
	buttons[i].pressed=0;
	buttons[i].wasPressed=0;
	buttons[i].held=0;
	buttons[i].holdCount=0;
	buttons[i].pressCount=0;
}
void bBusInit(){
	uchar i;
	for( i=0; i<N_BUTTONS; i++ ){
		buttons[i].pFunct=0;
		buttons[i].hFunct=0;
		buttons[i].dFunct=0;
		buttons[i].pressed=0;
		buttons[i].wasPressed=0;
		buttons[i].held=0;
		buttons[i].holdCount=0;
		buttons[i].pressCount=0;
		buttons[i].state=OFF;
	}
}

/*=================Function pointers as event listeners=======================*/

void bindPressEvent( uchar i, void (*f)() ){
	buttons[i].pFunct=f;
}
void bindHoldEvent( uchar i, void (*f)() ){
	buttons[i].hFunct=f;
}
void bindDoubleEvent( uchar i, void (*f)() ){
	buttons[i].dFunct=f;
}
void unbindAll(){
	uchar i;
	for(i=0; i<N_BUTTONS; i++){
		buttons[i].pFunct=0;
		buttons[i].hFunct=0;
		buttons[i].dFunct=0;
	}
}

/*=================Scheduled Tick=============================================*/

void buttonBus_tick(){
    static uchar pin;
    B++;
    if( B>=N_BUTTONS ){
	    B=0;
    }	

    pin=BIT( INBUS, B+B_OFFSET );
    switch (buttons[B].state){
        case INIT:
            buttons[B].state=OFF;
            break;
        case OFF:
            if(pin){
                if( buttons[B].pFunct ){//run bound function
	                buttons[B].pFunct();
                }
                if( buttons[B].dFunct && buttons[B].pressCount ){//run double click function
	                buttons[B].dFunct();
                }
                buttons[B].pressed=1;
				buttons[B].wasPressed=1;
				buttons[B].pressCount++;
                buttons[B].state=BOUNCEON;
            }
            break;
        case BOUNCEON:
			buttons[B].bounceCount++;
			buttons[B].holdCount++;
            if( THRESH_ON<buttons[B].bounceCount ){
                buttons[B].bounceCount=0;
                buttons[B].state=ON;
            }
            break;
        case ON:
            if(!pin){
                buttons[B].pressed=0;
                buttons[B].state=BOUNCEOFF;
            }
            break;
        case BOUNCEOFF:
            if( THRESH_OFF<buttons[B].bounceCount++ ){
                buttons[B].state=RESET;
            }
            break;
        case RESET:
            buttons[B].state=OFF;
            break;
        default:
            buttons[B].state=INIT;
            break;
    }
    switch (buttons[B].state){
        case INIT:
            buttons[B].bounceCount=0;
            buttons[B].holdCount=0;
            break;
        case OFF:
            break;
        case BOUNCEON:
            break;
        case ON:
			buttons[B].holdCount++;
            if( THRESH_HOLD<buttons[B].holdCount ){
                buttons[B].held=1;
				buttons[B].pressCount=0;
                if( buttons[B].hFunct ){//run bound function
	                buttons[B].hFunct();
                }
            }
            break;
        case BOUNCEOFF:
            break;
        case RESET:
            buttons[B].bounceCount=0;
            buttons[B].holdCount=0;
            buttons[B].state=OFF;
            break;
        default:
            break;
    }
}

/*=================Functions not used in this program=========================*/

/* Map the state of buttons in the array to an 8-bit value */
uchar bStateMap( uchar offset ){
	uchar i, out=0;
	for( i=0; i<N_BUTTONS; i++ ){
		if( buttons[i].pressed ){
			SET_BIT(out,i);
		}
	}
	return out<<offset;
}
uchar bHoldMap( uchar offset ){
	uchar i, out=0;
	for( i=0; i<N_BUTTONS; i++ ){
		if( buttons[i].held ){
			SET_BIT(out,i);
		}
	}
	return out<<offset;
}
