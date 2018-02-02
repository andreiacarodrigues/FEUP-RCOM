#ifndef _SHARED_H_
#define _SHARED_H_

#include <termios.h> //tcflag_t

#define DEBUG_MSGS 0

enum ConnectionMode
{
    TRANSMIT, RECEIVE
};
typedef enum ConnectionMode ConnectionMode;

void printByteByByte(char *str, unsigned int numBytes);

#endif //_SHARED_H_
