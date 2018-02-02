#ifndef _DATALINK_H_
#define _DATALINK_H_

#include <stdio.h> //perror
#include <unistd.h> //read

#include "shared.h"

//Ports
#define COM1 1
#define COM2 2
#define COM3 3

#define COM1_PORT_NAME "/dev/ttyS0"
#define COM2_PORT_NAME "/dev/ttyS1"
#define COM3_PORT_NAME "/dev/ttyS2"

#define PORT_NAME_SIZE 20

//Link layer info
typedef struct
{
    char port[PORT_NAME_SIZE];
    tcflag_t baudrate;
    size_t frameSize;
    unsigned char seqNum;
    unsigned int timeout;
    unsigned int maxTries;
    ConnectionMode mode;
    double byteErrorRatio;
} LinkLayerInfo;

//Link layer stats
typedef struct
{
    unsigned int IFramesSent;
    unsigned int IFramesReceived;

    unsigned int SETFramesSent;
    unsigned int SETFramesReceived;

    unsigned int DISCFramesSent;
    unsigned int DISCFramesReceived;

    unsigned int UAFramesSent;
    unsigned int UAFramesReceived;

    unsigned int RRFramesSent;
    unsigned int RRFramesReceived;

    unsigned int REJFramesSent;
    unsigned int REJFramesReceived;

    unsigned int timeouts;

    unsigned int badSize;
    unsigned int badA;
    unsigned int badC;
    unsigned int badBCC1;
    unsigned int badBCC2;
} LinkLayerStats;

//Frame types
enum FrameType
{
    INFO, SET, DISC, UA, RR, REJ, BAD
};
typedef enum FrameType FrameType; //so we can write FrameType instead of enum FrameType

//Frame designations
enum FrameDesignation
{
    FD_CMD, FD_REP
};
typedef enum FrameDesignation FrameDesignation;

//Frame types
enum ErrorType
{
    EMPTY, BAD_SIZE, BAD_SIZE_TWO, BAD_BCC1, BAD_A, BAD_C, BAD_SEQ_NUM, BAD_BCC2
};
typedef enum ErrorType ErrorType;

typedef struct
{
    ErrorType error;
    FrameType type;
    char *packet;
    size_t packetSize;
    unsigned char seqNum;
} ReceivedFrameInfo;

//Important bytes in frames
#define FLAG 0x7E
#define FLAG_STUFF 0x5E
#define ESCAPE 0x7D
#define ESCAPE_STUFF 0x5D

//Address field (AF) bytes
#define TRANSMITTER_CMD 0x03
#define TRANSMITTER_REP 0x01
#define RECEIVER_CMD 0x01
#define RECEIVER_REP 0x03

//Control field (CF) bytes
#define INFO_BASE_CF 0x00
#define SET_CF 0x03
#define DISC_CF 0x0B
#define UA_CF 0x07
#define RR_BASE_CF 0x05
#define REJ_BASE_CF 0x01

//Frame sizes
#define SU_FRAME_SIZE 5
#define I_FRAME_BASE_SIZE 6

ssize_t readFromFd(int fd, char *buf);

//Services provided to the application layer
int llopen(int port, ConnectionMode mode);
int llwrite(int fd, char *buffer, size_t length);
ssize_t llread(int fd, char **buffer);
int llclose(int fd);

//Building/Destroying the llInfo
int setPort(int port);
int setTermios(int fd);
int initDatalinkLayer(tcflag_t baudrate, size_t frameSize, unsigned int timeout, unsigned int maxTries,
                      double byteErrorRatio);

//Stats
void resetLlStats();
void printLlXmitStats();
void printLlRcvrStats();

//Sending frames
int sendSUFrame(int fd, FrameType type);

int sendIFrame(int fd, char *data, size_t length);

//Receiving frames
int receiveFrame(int fd, ReceivedFrameInfo *rInfo);
int processFrame(char *data, size_t dataSize, ReceivedFrameInfo *rInfo);

//Building information frames (I frames)
char *buildIFrame(char *data, unsigned int length);

//Building non-information frames (S and U frames)
char *buildSUFrame(FrameType type);

FrameDesignation getFrameDesignation(FrameType type);
char getAddressField(FrameType type);
char getControlField(FrameType type);

//Stuffing/destuffing frames
unsigned int stuff(char **frame, unsigned int frameSize);

unsigned int destuff(char **frame, unsigned int frameSize);

//Random Error Inserting
void insertRandomErrors(char *buffer, size_t length);

//Baudrate values
#define NO_BS {50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800}
#define BS {B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800}

tcflag_t getBaudrate(int rate);
void printBaudrates();

#endif //_DATALINK_H_
