#ifndef LCDDUAL_H
#define LCDDUAL_H

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long 

/* 'public' functions */
void msgWrite( uchar wScreen, char buf[], unsigned char setTimeout );
uchar fullQ( uchar wScreen );
uchar emptyQ();
uchar numInQ( uchar wScreen );
uchar msgIsLocked( uchar wScreen );
void msgClear( uchar wScreen );
void dispWelcomeScreen( uchar wScreen );

int message_tick( int state );

void lcdTest();

#endif