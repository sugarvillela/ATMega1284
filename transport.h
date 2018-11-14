#ifndef METRONOME_H_
#define METRONOME_H_

#define BEEP_PERIOD 5
#define MET_PERIOD 50

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
void beepPref( unsigned char onOff );

/* Accessors */
void getTimeSig_str( unsigned char buf[] );		// 4:4, 3:4 or 6:8 as string
void getTempo_str( unsigned char buf[] );		// nToChar current tempo
unsigned char getBeepOn();
void getTimeMB( unsigned char buf[] );

int metronome_tick( int state );
int beep_tick( int state );

#endif /* METRONOME_H_ */