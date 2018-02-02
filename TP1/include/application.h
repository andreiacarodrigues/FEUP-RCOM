#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <stdio.h> //perror
#include <unistd.h> //read
#include <stdlib.h> //malloc
#include <string.h> //strncpy

#include "shared.h"

#define PORT_NAME_SIZE 20
#define MAX_BUF 256

//Application layer info
typedef struct
{
    int fd;    /*Descritor correspondente à porta série*/
    int port;
    char fileName[256];
    unsigned int fileSize;
    ConnectionMode mode;
    unsigned int seqNum;
    size_t packetSize;
} ApplicationLayerInfo;

//Application layer stats
typedef struct
{
    unsigned int CPacketsSent;
    unsigned int CPacketsReceived;

    unsigned int DPacketsSent;
    unsigned int DPacketsReceived;
} ApplicationLayerStats;

//Packet types
enum PacketType
{
    PKT_DATA, PKT_START, PKT_END
};
typedef enum PacketType PacketType;

#define PACKET_TYPE_DATA 1
#define PACKET_TYPE_START 2
#define PACKET_TYPE_END 3

//Packet sizes
#define D_PACKET_BASE_SIZE 4
#define C_PACKET_BASE_SIZE 5

#define FILE_SIZE_TYPE 0
#define FILE_SIZE_SIZE 4
#define FILE_NAME_TYPE 1

#define MAX_CHAR 256

//Init Application Layer
int initApplicationLayer(int port, tcflag_t baudrate, size_t frameSize, unsigned int timeout, unsigned int maxTries,
                         double byteErrorRatio);

//Stats
void resetAlStats();
void printAlXmitStats();
void printAlRcvrStats();

//Get file size
off_t getFileSize(int fd);

//File transmission
int sendFile(char *fileName);
void printSendFileInfo(double secsElapsed, unsigned int bytesSent);

void printProgressBar(double ratio);

//File reception
int receiveFile();
void printEarlyReceiveFileInfo(double secsElapsed);
void printReceiveFileInfo(double secsElapsed, unsigned int bytesReceived);

//Sending packets
int sendDPacket(unsigned int dataSize, char *data);
int sendCPacket(PacketType type);

//Receiving packets
int receiveDPacket(unsigned int *dataSize, char **data);
int receiveCPacket(PacketType *type);

//Processing packets
int processDPacket(char *packet, unsigned int *dataSize, char **data);
int processCPacket(char *packet, PacketType *type);

//Building a data packet  
char *buildDPacket(unsigned int dataSize, char *data);

//Building a control packet 
char *buildCPacket(PacketType type);

#endif //_APPLICATION_H_
