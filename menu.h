#ifndef MENU_H
#define MENU_H

#define uchar unsigned char
#define ushort unsigned short
#define uint unsigned int
#define ulong unsigned long 

/* 'public' functions */
typedef struct menuStruct mStruct;
struct menuStruct{
    char name[9];
    mStruct* left;
    mStruct* right;
    mStruct* up;
    mStruct* down;
    void (*value)( uchar buf[] );
    void (*go)();
};

void menuInit();
void menuOn();
void menuOff();
void menuWrite();

#endif //MENU_H