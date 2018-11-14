#ifndef BUTTONBUS_H
#define BUTTONBUS_H

#define	B_BUS_PERIOD	125	//5 milliseconds

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long


/* 'public' functions */
/* accessors */
uchar bPressed( uchar i );
uchar bWasPressed( uchar i );
uchar bHeld( uchar i );
ushort bHoldTime( uchar i );
ushort bEffHoldTime( uchar i );
uchar bPressCount( uchar i );
uchar bStateMap( uchar offset );
uchar bHoldMap( uchar offset );


/* Mutators */
void bBusInit();
void bReset( uchar i );
//void zeroAllBut( uchar button );

void bindPressEvent( uchar i, void (*f)() );
void bindHoldEvent( uchar i, void (*f)() );
void bindDoubleEvent( uchar i, void (*f)() );
void unbindAll();

int buttonBus_tick( int state );
//
//int buttonBus_testTick( int state );
//void buttonBus_test();

#endif //BUTTONBUS_H