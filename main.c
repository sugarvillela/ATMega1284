#include <avr/io.h>
#include <avr/interrupt.h>
//#include <unistd.h>

#include "UT.h"	//getGCD function moved here
#include "scheduler.h"	
//#include "usart_ATmega1284.h"
//#include "spi.h"
//#include "readWrite.h"
#include "lcddual.h"
#include "shiftreg16.h"
#include "ledmatrix.h"
#include "buttonbus.h"
#include "transport.h"
#include "refclock.h"
#include "menu.h"
#include "userin.h"
//#include "keypad.h"
//#include "queue.h"

uchar dataD;//port B to send
enum blink_States { BLINK_ON, BLINK_OFF, BLINK_IDLE };
int blink_tick( int state ) {//Blinker
	switch (state) {//states
		case BYP:
			return state;
		case BLINK_ON:
			state=BLINK_OFF;
			break;
		case BLINK_OFF:
			state=BLINK_ON;
			break;
		default:
			state=BLINK_ON;
			break;
	}
	switch (state) {//action
		case BLINK_ON:
			dataD = 0x01;
			break;
		case BLINK_OFF:
			dataD = 0;
			break;
		default:
			break;
	}
	PORTD = dataD;//(PORTD&0xF)|dataD;
	return state;
}
int pinOut_tick( int state ){
	static ushort toReg=0x01;
	static uchar count=0;
	if( state==BYP ){ return state; }
	if(count==0){
		sendToReg( toReg );
	}
	count++;
	if(count>=40){
		//messageSet_num( toReg, "toReg", 15 );
		toReg++;
		count=0;
	}
	//PORTB=dataD & 0x0F;
	//PORTD=( dataD >> 4 ) & 0x0F;
	////dataD=0;
	return state;
}
void voidFunct(){
	msgWrite( 0, "functCalled!", MSG_LONG );
}
#define NUM_TASKS	10

int main(void){//DDR 0 for input, F for output
	tasksNum = NUM_TASKS;//don't delete this!!!!
	setIO(
		"iiiiiiii",	//IO A	LSB on Right!
		"oooooooo",	//IO B
		"oooooooo",	//IO C
		"oooooooo"	//IO D
	);
	ulong int GCD;

	task tsks[NUM_TASKS];
	tasks = tsks;
	
	uchar i = 0;
	/* reference clock */
	tasks[i].state = -1;
	tasks[i].period = REFCLK_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &refclock_tick;
	i++;
	/* metronome */
	tasks[i].state = -1;
	tasks[i].period = MET_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &metronome_tick;
	i++;
	/* metronome beep*/
	tasks[i].state = -1;
	tasks[i].period = BEEP_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &beep_tick;
	i++;
	/* tap clock */
	tasks[i].state = -1;
	tasks[i].period = REFCLK_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &tap_tick;
	i++;
	///* matrix draw */
	//tasks[i].state = -1;
	//tasks[i].period = 24;
	//tasks[i].elapsedTime = tasks[i].period;
	//tasks[i].TickFct = &matrix_tick;
	//i++;
	///* matrix patterns */
	//tasks[i].state = -1;
	//tasks[i].period = 1000;
	//tasks[i].elapsedTime = tasks[i].period;
	//tasks[i].TickFct = &pat_tick;
	//i++;
	///* shift register */
	//tasks[i].state = -1;
	//tasks[i].period = 1;
	//tasks[i].elapsedTime = tasks[i].period;
	//tasks[i].TickFct = &shro_tick;
	//i++;
	/* message_tick */
	tasks[i].state = -1;
	tasks[i].period = MSG_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &message_tick;
	i++;
	/* message_tick */
	tasks[i].state = -1;
	tasks[i].period = MSG_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &message_tick;
	i++;
	/* Buttons */
	tasks[i].state = -1;//Task initial state.
	tasks[i].period = B_BUS_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonBus_tick;
	i++;
	tasks[i].state = -1;//Task initial state.
	tasks[i].period = B_BUS_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonBus_tick;
	i++;
	tasks[i].state = -1;//Task initial state.
	tasks[i].period = B_BUS_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonBus_tick;
	i++;
	tasks[i].state = -1;//Task initial state.
	tasks[i].period = B_BUS_PERIOD;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &buttonBus_tick;
	i++;	
	///* pinOut_tick */
	//tasks[i].state = BYP;
	//tasks[i].period = 100;
	//tasks[i].elapsedTime = tasks[i].period;
	//tasks[i].TickFct = &pinOut_tick;
	
	GCD = tasks[0].period;
	for( i=1; i<NUM_TASKS; i++){
		GCD = findGCD( GCD, tasks[i].period );
	}
	//shiftRegInit();
	//matrixInit();
	bBusInit();
	transportInit();
	//transportOn();
	//transportTop();
	uinInit();
	msgInit();
	msgBindDropEvent( 0, &startupDisplay );
	dispWelcomeScreen( 1 );
	menuInit();//initialize this last
	TimerSet( GCD );
	TimerOn();
	while(1) {}
}
