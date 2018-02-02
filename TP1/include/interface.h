#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include "shared.h"

//Communication Configuration
typedef struct
{
    int port;
    long int uiBaudrate;
    tcflag_t baudrate;
    size_t frameSize;
    unsigned int timeout;
    unsigned int maxTries;
    double byteErrorRatio;
} CommunicationConfig;

void initCommunicationConfig();

//Reading input
int readLongInt(long int *li);
int readDouble(double *d);

void pressEnter();

//Main menu
void printMainMenu();

void printWelcomeMessage();

void printConfig();

void printMainMenuOptions();

void interfaceSendFile();

void interfaceReceiveFile();

void readPort();

void readBaudrate();

void readFrameSize();

void readTimeout();

void readMaxTries();

void readByteErrorRatio();

#endif //_INTERFACE_H_
