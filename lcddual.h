#ifndef LCDDUAL_H
#define LCDDUAL_H

#define	MSG_PERIOD	125	//	25 milliseconds
#define MSG_SHORT	20	//	0.5 seconds
#define MSG_LONG	80	//	2 seconds
#define FOREVER		255

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long

/* 'public' functions */
void msgInit();
/* Set timeout use defines above or any number < 254 */
void msgWrite( uchar wScreen, char buf[], unsigned char setTimeout );
void msgWrite_stack( uchar wScreen, char buf1[], char buf2[], uchar setTimeout );
void msgWrite_num( uchar wScreen, int format, char label[], ushort num, uchar setTimeout );
uchar fullQ( uchar wScreen );
uchar emptyQ();
uchar numInQ( uchar wScreen );
uchar msgIsLocked( uchar wScreen );
void msgClear( uchar wScreen );
void dispWelcomeScreen( uchar wScreen );

void msgBindDropEvent( uchar wScreen, void (*f)() );
void msgUnbindDropEvent( uchar wScreen );
void msgUnbindAll();

/* For busy messaging where queue is too fancy for ATMega */
void msgQueueOn( uchar wScreen );
void msgQueueOff( uchar wScreen );
void simpleWrite( uchar wScreen, char buf[] );
void simpleWrite_num( uchar wScreen, int format, ushort num );
void simpleWrite_float( uchar wScreen, float num );
void simpleWrite_13( uchar wScreen, char buf1[], char buf2[] );
void simpleWrite_123( uchar wScreen, char buf1[], char buf2[], char buf3[] );
void simpleWrite_1234( uchar wScreen, char buf1[], char buf2[], char buf3[], char buf4[] );
void simpleClear( uchar wScreen );

int message_tick( int state );
void setScreen( uchar scr );

void LCD_init(void);
void LCD_ClearScreen( uchar wScreen );
void LCD_WriteCommand (uchar Command, uchar wScreen  );
void LCD_WriteData( uchar Data, uchar wScreen );
void LCD_Disp( uchar column, const uchar* string, uchar wScreen );//no clear screen
void LCD_DispNew( uchar column, const uchar* string, uchar wScreen );//with clear screen

void LCD_Cursor( uchar column,  uchar wScreen );

#endif