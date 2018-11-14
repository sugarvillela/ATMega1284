#ifndef USERIN_H_
#define USERIN_H_

/*==========================User in: character entry==========================*/

void uinSetLabel( char label[] );

void uinOn();
void uinOff();

/*==========================User in: numeric entry============================*/

void numinSetLabel( char label[] );
void numinSetDef( unsigned char setVal, unsigned char setBound );
void numinOn();
void numinOff();

/*==========================User in: tap tempo================================*/

void tapOn();
void tapOff();
int tap_tick( int state );
/*==========================Shared Initializer================================*/

void uinInit();

#endif /* USERIN_H_ */