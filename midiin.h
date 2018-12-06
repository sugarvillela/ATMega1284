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

#ifndef MIDIIN_H_
#define MIDIIN_H_

/*========================================================================================
	USART
==========================================================================================*/

#define F_CPU 8000000UL // Assume uC operates at 8MHz
#define BAUD_RATE 31250
#define BAUD_PRESCALE (((F_CPU / (BAUD_RATE * 16UL))) - 1)

void initUSART(unsigned char usartNum);
unsigned char USART_IsSendReady(unsigned char usartNum);
unsigned char USART_HasTransmitted(unsigned char usartNum);
unsigned char USART_HasReceived(unsigned char usartNum);
void USART_Flush(unsigned char usartNum);
void USART_Send(unsigned char sendMe, unsigned char usartNum);
unsigned char USART_Receive(unsigned char usartNum);

/*========================================================================================
	Record, Play, Display and Time-Manipulate incoming midi data
==========================================================================================*/

#define MIDI_PERIOD     1			//200 microseconds
#define MIDI_SIZE       250         //size of incoming data buffer

/*===============Recording and Playback=============================================*/

void midiRec();
void midiPlay();
void midiOff();
void midiTop();
void midiSeek( unsigned short t );

unsigned char midiFull();
void midiInit();
void midi_tick( unsigned short t );
void midiClockOverride( unsigned short t );

/*===============Event List Display=================================================*/

void eventDispUpdate();
void eventUp();
void eventDown();
void eventUpSkip();
void eventDownSkip();

void eventDispOn();
void eventDispOff();
void eventStatus( unsigned char buf[] );
unsigned char haveEvents();

void testFill();

/*===============Data Manipulation==================================================*/

void writeNewTempo( unsigned char oldTempo, unsigned char newTempo );
void quantize( unsigned char tempo, unsigned char tsig2, unsigned char res );

/*===============Logistics==========================================================*/

void midiBindOffEvent( void (*f)() );
void midiUnbindAll();

/*===============Iterator and Loader for file save==================================*/

/* Event iterator */
void itrReset();
unsigned char itrDone();
unsigned short itrNext();

/* Event loader */
void loaderReset();
unsigned char loaderDone();
void load( unsigned short d );
void loaderFinalize();

void midiToFile();
void midiFromFile();

#endif /* MIDIIN_H_ */