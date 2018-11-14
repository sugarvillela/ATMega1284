//#include <stdio.h>
#include <avr/io.h>
#include "buttonbus.h"
#include "UT.h"
#include "lcddual.h"

/*  A tool for momentary buttons on a single bus A, B, C or D:
    Define an array for button states pressed and held 
    Tick functions resets 'pressed' field.
    Conversely, 'held' field' must be reset by another part of the program 
    (the one waiting for a 'held' event calls reset upon receipt of the event)
    Buttons must be consecutively placed, but not from 0 (use B_OFFSET).
	
    IMPORTANT: the way tick-sharing works is you make a task for each button.
    The tick function increments the button just before it returns so the next
    tick accesses the next button.  Make sure N_BUTTONS = number of tasks that
    use the tick function
*/



#define N_BUTTONS       4       //number of buttons (see above)
#define INBUS           PINA    //input bus
#define B_OFFSET        1       //buttons don't have to start at pin 0
#define OUTBUS			PORTD

#define THRESH_ON		3		//12*5=60ms;  60 on 60 off = 120ms 
#define THRESH_OFF		3		
#define THRESH_HOLD		50		//250*5=1.25 seconds

typedef struct {
    uchar bounceCount;			//tick counter for debouncing
    ushort holdCount;			//tick counter for comparing to THRESH_HOLD
    uchar pressed;				// resets on button release
	uchar wasPressed;			// persistent until checked, reset or zeroAllBut
	uchar held;					// persistent until checked, reset or zeroAllBut
	uchar pressCount;			// persistent until checked, reset or zeroAllBut
	void (*pFunct)();
	void (*hFunct)();
	void (*dFunct)();
} buttonStruct;

buttonStruct buttons[N_BUTTONS];

/* 'public' functions */
/* accessors */

uchar bPressed( uchar i ){
    return (i<N_BUTTONS)? buttons[i].pressed : 0;
}
uchar bWasPressed( uchar i ){
	//add code to reset
	return (i<N_BUTTONS)? buttons[i].wasPressed : 0;
}
uchar bHeld( uchar i ){
	//add code to reset
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
    if(i<N_BUTTONS){
		buttons[i].pressed=0;
		buttons[i].wasPressed=0;
		buttons[i].held=0;
		buttons[i].holdCount=0;
		buttons[i].pressCount=0;
    }
}
//void zeroAllBut( uchar button ){
	//uchar i;
	//for( i=0; i<N_BUTTONS; i++ ){
		//if( button!=i ){
			//buttons[i].pressed=0;
			//buttons[i].wasPressed=0;
			//buttons[i].held=0;
			////buttons[i].holdCount=0;
			//buttons[i].effHold=0;
			//buttons[i].pressCount=0;
		//}
	//}
//}
void bindPressEvent( uchar i, void (*f)() ){
	if(i<N_BUTTONS){
		buttons[i].pFunct=f;
	}
}
void bindHoldEvent( uchar i, void (*f)() ){
	if(i<N_BUTTONS){
		buttons[i].hFunct=f;
	}
}
void bindDoubleEvent( uchar i, void (*f)() ){
	if(i<N_BUTTONS){
		buttons[i].dFunct=f;
	}
}

//void unbindAll(){
	//uchar i;
	//for(i=0; i<N_BUTTONS; i++){
		//buttons[i].pFunct=0;
		//buttons[i].hFunct=0;
		//buttons[i].dFunct=0;
	//}
//}
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
	}
}



enum bstates{ B_INIT, B_OFF, B_BOUNCEON, B_ON, B_BOUNCEOFF, B_RESET };
int buttonBus_tick( int state ){
    if( state==BYP ){ return state; }
    static uchar pin;
    static uchar button=0;

    pin=BIT( INBUS, button+B_OFFSET );
    switch (state){
        case B_INIT:
            state=B_OFF;
            break;
        case B_OFF:
            if(pin){
				//zeroAllBut( button );//assuming we don't need simultaneous press events
                if( buttons[button].pFunct ){//run bound function
	                buttons[button].pFunct();
                }
                if( buttons[button].dFunct && buttons[button].pressCount ){//run double click function
					buttons[button].pressCount=0;
	                buttons[button].dFunct();
                }
                buttons[button].pressed=1;
				buttons[button].wasPressed=1;
				buttons[button].pressCount++;
                state=B_BOUNCEON;
				OUTBUS=(OUTBUS & 0xF0) | bStateMap(0);
            }
            break;
        case B_BOUNCEON:
			buttons[button].bounceCount++;
			buttons[button].holdCount++;
            if( THRESH_ON<buttons[button].bounceCount ){
                buttons[button].bounceCount=0;
                state=B_ON;
            }
            break;
        case B_ON:
            if(!pin){
                buttons[button].pressed=0;
                state=B_BOUNCEOFF;
				OUTBUS=(OUTBUS & 0xF0) | bStateMap(0);
				
            }
            break;
        case B_BOUNCEOFF:
            if( THRESH_OFF<buttons[button].bounceCount++ ){
                state=B_RESET;
            }
            break;
        case B_RESET:
            state=B_OFF;
            break;
        default:
            state=B_INIT;
            break;
    }
    switch (state){
        case B_INIT:
            buttons[button].bounceCount=0;
            buttons[button].holdCount=0;
            break;
        case B_OFF:
            break;
        case B_BOUNCEON:
            break;
        case B_ON:
			buttons[button].holdCount++;
            if( THRESH_HOLD<buttons[button].holdCount ){
                buttons[button].held=1;
				//buttons[button].holdCount=holdCounts[button];
				buttons[button].pressCount=0;
                if( buttons[button].hFunct ){//run bound function
	                buttons[button].hFunct();
                }
            }
            break;
        case B_BOUNCEOFF:
            break;
        case B_RESET:
            buttons[button].bounceCount=0;
            buttons[button].holdCount=0;
            state=B_OFF;
            break;
        default:
            break;
    }
    button++;
    if( button==N_BUTTONS ){
        button=0;
    }
    return state;
}

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
