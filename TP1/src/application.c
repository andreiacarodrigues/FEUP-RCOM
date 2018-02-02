#include <libgen.h>
#include <sys/times.h> //times
#include <sys/fcntl.h>
#include "application.h"
#include "datalink.h"
#include "util_interface.h"

static ApplicationLayerInfo alInfo;
static ApplicationLayerStats alStats;

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) // 0666

//Init Application Layer
int initApplicationLayer(int port, tcflag_t baudrate, size_t frameSize, unsigned int timeout, unsigned int maxTries,
                         double byteErrorRatio)
{
    alInfo.port = port;
    alInfo.packetSize = frameSize - 6;

    if (initDatalinkLayer(baudrate, frameSize, timeout, maxTries, byteErrorRatio / 100) == -1)
    {
        return -1;
    }

    return 0;
}

//Stats
void resetAlStats()
{
    bzero(&alStats, sizeof(alStats));
}

void printAlXmitStats()
{
    printf(" --- Application Layer Stats ---\n");

    printf(YEL "C Packets Sent: ");
    printf(CYN "%u\n", alStats.CPacketsSent);

    printf(YEL "D Packets Sent: ");
    printf(CYN "%u\n" RESET, alStats.DPacketsSent);
}

void printAlRcvrStats()
{
    printf(" --- Application Layer Stats ---\n");

    printf(YEL "C Packets Received: ");
    printf(CYN "%u\n", alStats.CPacketsReceived);

    printf(YEL "D Packets Received: ");
    printf(CYN "%u\n" RESET, alStats.DPacketsReceived);
}

//Get file size
off_t getFileSize(int fd)
{
    off_t fileSize;

    off_t currentOffset = lseek(fd, 0, SEEK_CUR);

    fileSize = lseek(fd, 0, SEEK_END);

    lseek(fd, currentOffset, SEEK_SET);

    return fileSize;
}

//File transmission
int sendFile(char *fileName)
{
    clock_t start = times(NULL);
    const long int ticks_per_sec = sysconf(_SC_CLK_TCK);

    resetAlStats();

    //Step 1: Begin the connection at the datalink layer
    alInfo.mode = TRANSMIT;
    alInfo.fd = llopen(alInfo.port, TRANSMIT);
    if (alInfo.fd == -1)
    {
        printf("Failed to establish connection: llopen failed.\n");
        return -1;
    }

    //Step 2: Open the file to send
    int fd = open(fileName, O_RDONLY);
    if (-1 == fd)
    {
        perror("open");
        llclose(alInfo.fd);
        return -1;
    }
    strcpy(alInfo.fileName, basename(fileName));

    //Step 3: Get the file's size
    alInfo.fileSize = (unsigned int) getFileSize(fd);
    printf("sendFile: fileSize - %u\n", alInfo.fileSize);
    if (alInfo.fileSize == -1)
    {
        perror("getFileSize");
        close(fd);
        llclose(alInfo.fd);
        return -1;
    }

    unsigned int NSent = 0;
    clock_t end = times(NULL);
    double secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printSendFileInfo(secsElapsed, NSent);
    }

    //Step 4: Send the start control packet
    if (sendCPacket(PKT_START) == -1)
    {
        perror("sendCPacket");
        close(fd);
        llclose(alInfo.fd);
        return -1;
    }
    alStats.CPacketsSent++;

    end = times(NULL);
    secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printSendFileInfo(secsElapsed, NSent);
    }

    //Step 5: Send the file, chunk by chunk
    const size_t MAX_CHUNK_SIZE = alInfo.packetSize - 4;
    char fileBuf[MAX_CHUNK_SIZE]; //buffer for file chunks
    alInfo.seqNum = 0;
    ssize_t nRead;
    while ((nRead = read(fd, fileBuf, MAX_CHUNK_SIZE)) > 0)
    {
        unsigned int nReadUInt = (unsigned int) nRead;
        if (sendDPacket(nReadUInt, fileBuf) == -1)
        {
            close(fd);
            llclose(alInfo.fd);
            return -1;
        }
        alStats.DPacketsSent++;


        end = times(NULL);
        secsElapsed = (double) (end - start) / ticks_per_sec;
        if (!DEBUG_MSGS)
        {
            printSendFileInfo(secsElapsed, NSent);
        }
        NSent += nReadUInt;
    }

    if (-1 == nRead)
    {
        perror("read");
        close(fd);
        llclose(alInfo.fd);
        return -1;
    }

    //Step 6: Send the end control packet
    if (sendCPacket(PKT_END) == -1)
    {
        close(fd);
        llclose(alInfo.fd);
        return -1;
    }
    alStats.CPacketsSent++;

    end = times(NULL);
    secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printSendFileInfo(secsElapsed, NSent);
    }

    //Step 7: Close the file to send
    if (close(fd) != 0)
    {
        perror("close");
        llclose(alInfo.fd);
        return -1;
    }

    //Step 8: End the connection at the datalink layer
    if (llclose(alInfo.fd) == -1)
    {
        return -1;
    }

    end = times(NULL);
    secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printSendFileInfo(secsElapsed, NSent);
    }

    return 0;
}

void printSendFileInfo(double secsElapsed, unsigned int bytesSent)
{
    clearScreen();

    static unsigned char nDots = 1;
    printf("The file is being sent");
    int i;
    for (i = 0; i < nDots; i++)
    {
        printf(".");
    }
    printf("\n");
    nDots++;
    const unsigned char MAX_DOTS = 3;
    if (nDots > MAX_DOTS)
    {
        nDots = 1;
    }

    printf(YEL "Time elapsed: ");
    printf(CYN "%f s.\n", secsElapsed);

    printf(YEL "Filename: ");
    printf(CYN "%s.\n", alInfo.fileName);

    double progress = (double) bytesSent / alInfo.fileSize;
    printf(YEL "Bytes sent: ");
    printf(CYN "%u/%u (%f%%).\n" RESET, bytesSent, alInfo.fileSize, 100 * progress);
    printProgressBar(progress);

    printf("\n");

    printLlXmitStats();
    printAlXmitStats();
}

void printProgressBar(double ratio)
{
    double remaining = ratio;
    while (remaining > 0)
    {
        if (ratio < 0.5)
        {
            printf(RED_BG " " RESET);
        }
        else if (ratio < 1)
        {
            printf(YEL_BG " " RESET);
        }
        else
        {
            printf(GRN_BG " " RESET);
        }
        remaining -= 0.02;
    }
}

//File reception
int receiveFile()
{
    clock_t start = times(NULL);
    const long int ticks_per_sec = sysconf(_SC_CLK_TCK);

    resetAlStats();

    clock_t end = times(NULL);
    double secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printEarlyReceiveFileInfo(secsElapsed);
    }

    if (DEBUG_MSGS)
    { printf("ReceiveFile: AA\n"); }
    //Step 1: Begin the connection at the datalink layer
    alInfo.mode = RECEIVE;
    alInfo.fd = llopen(alInfo.port, RECEIVE);
    if (alInfo.fd == -1)
    {
        printf("Failed to establish connection: llopen failed.\n");
        return -1;
    }

    if (DEBUG_MSGS)
    { printf("ReceiveFile: BB\n"); }
    //Step 2: Receive the start packet
    PacketType type;
    if (receiveCPacket(&type) == -1)
    {
        printf("ReceiveFile: BB1\n");
        return -1;
    }
    if (type != PKT_START)
    {
        printf("ReceiveFile: BB2\n");
        return -1;
    }
    alStats.CPacketsReceived++;

    unsigned int nWrote = 0;
    end = times(NULL);
    secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printReceiveFileInfo(secsElapsed, nWrote);
    }

    if (DEBUG_MSGS)
    { printf("ReceiveFile: CC\n"); }
    //Step 3: Open the file to store the data received
    int fd = open(alInfo.fileName, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
    if (-1 == fd)
    {
        perror("open");
        llclose(alInfo.fd);
        return -1;
    }

    if (DEBUG_MSGS)
    { printf("ReceiveFile: DD\n"); }
    //Step 4: Receive the file, chunk by chunk
    char *fileBuf = (char *) malloc(sizeof(char) * MAX_BUF); //buffer for file chunks
    if (fileBuf == NULL)
    {
        perror("malloc");
        llclose(alInfo.fd);
        return -1;
    }
    alInfo.seqNum = 0;
    unsigned int nReceived;
    while (nWrote < alInfo.fileSize)
    {
        if (-1 == receiveDPacket(&nReceived, &fileBuf))
        {
            free(fileBuf);
            close(fd);
            llclose(alInfo.fd);
            return -1;
        }
        alStats.DPacketsReceived++;

        nWrote += nReceived;

        if (write(fd, fileBuf, nReceived) != nReceived)
        {
            perror("write");
            free(fileBuf);
            close(fd);
            llclose(alInfo.fd);
            return -1;
        }

        end = times(NULL);
        secsElapsed = (double) (end - start) / ticks_per_sec;
        if (!DEBUG_MSGS)
        {
            printReceiveFileInfo(secsElapsed, nWrote);
        }
    }
    free(fileBuf);

    if (DEBUG_MSGS)
    { printf("ReceiveFile: EE\n"); }
    //Step 5: Close the output file
    if (close(fd) != 0)
    {
        perror("fclose");
        llclose(alInfo.fd);
        return -1;
    }

    end = times(NULL);
    secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printReceiveFileInfo(secsElapsed, nWrote);
    }

    if (DEBUG_MSGS)
    { printf("ReceiveFile: FF\n"); }
    //Step 6: Receive the end control packet
    if (receiveCPacket(&type) == -1)
    {
        llclose(alInfo.fd);
        return -1;
    }
    if (type != PKT_END)
    {
        llclose(alInfo.fd);
        return -1;
    }
    alStats.CPacketsReceived++;

    end = times(NULL);
    secsElapsed = (double) (end - start) / ticks_per_sec;
    if (!DEBUG_MSGS)
    {
        printReceiveFileInfo(secsElapsed, nWrote);
    }

    if (DEBUG_MSGS)
    { printf("ReceiveFile: GG\n"); }
    //Step 7: End the connection at the datalink layer
    if (llclose(alInfo.fd) == -1)
    {
        return -1;
    }

    return 0;
}

void printEarlyReceiveFileInfo(double secsElapsed)
{
    clearScreen();

    printf("Waiting for connection with the transmitter...\n");
}

void printReceiveFileInfo(double secsElapsed, unsigned int bytesReceived)
{
    clearScreen();

    static unsigned char nDots = 1;
    printf("The file is being received");
    int i;
    for (i = 0; i < nDots; i++)
    {
        printf(".");
    }
    printf("\n");
    nDots++;
    const unsigned char MAX_DOTS = 3;
    if (nDots > MAX_DOTS)
    {
        nDots = 1;
    }

    printf(YEL "Time elapsed: ");
    printf(CYN "%f s.\n", secsElapsed);

    printf(YEL "Filename: ");
    printf(CYN "%s.\n", alInfo.fileName);

    double progress = (double) bytesReceived / alInfo.fileSize;
    printf(YEL "Bytes received: ");
    printf(CYN "%u/%u (%f%%).\n" RESET, bytesReceived, alInfo.fileSize, 100 * progress);
    printProgressBar(progress);

    printf("\n");

    printLlRcvrStats();
    printAlRcvrStats();
}

//Sending packets
int sendDPacket(unsigned int dataSize, char *data)
{
    char *packet = buildDPacket(dataSize, data);
    if (packet == NULL)
    {
        return -1;
    }

    if (llwrite(alInfo.fd, packet, dataSize + D_PACKET_BASE_SIZE) == -1)
    {
        printf("Failed to send a D packet: llwrite failed.\n");
        free(packet);
        return -1;
    }

    free(packet);
    alInfo.seqNum++;
    return 0;
}

int sendCPacket(PacketType type)
{
    char *packet = buildCPacket(type);
    if (packet == NULL)
    {
        return -1;
    }

    const unsigned char fileNameSize = strlen(alInfo.fileName);
    if (llwrite(alInfo.fd, packet, FILE_SIZE_SIZE + fileNameSize + C_PACKET_BASE_SIZE) == -1)
    {
        printf("Failed to send a C packet: llwrite failed.\n");
        free(packet);
        return -1;
    }

    free(packet);
    return 0;
}

//Receiving packets
int receiveDPacket(unsigned int *dataSize, char **data)
{
    if (DEBUG_MSGS)
    {
        printf("receiveDPacket called.\n");
    }
    char *packet;
    if (-1 == llread(alInfo.fd, &packet))
    {
        return -1;
    }

    int processDPac = processDPacket(packet, dataSize, data);
    free(packet);
    if (processDPac == -1)
    {
        return -1;
    }
    if (processDPac == -2)
    {
        *dataSize = 0;
        return 0;
    }
    alInfo.seqNum = (alInfo.seqNum + 1) % MAX_CHAR;
    if (DEBUG_MSGS)
    {
        printf("receiveDPacket success.\n");
    }
    return 0;
}

int receiveCPacket(PacketType *type)
{
    char *packet;
    if (llread(alInfo.fd, &packet) == -1)
    {
        return -1;
    }

    if (processCPacket(packet, type) == -1)
    {
        free(packet);
        return -1;
    }

    free(packet);

    return 0;
}

//Processing packets
int processDPacket(char *packet, unsigned int *dataSize, char **data)
{
    if (DEBUG_MSGS)
    {
        printf("processDPacket called.\n");
    }
    char packetType = packet[0];
    if (packetType != PACKET_TYPE_DATA)
    {
        if (DEBUG_MSGS)
        {
            printf("processDPacket Bad1.\n");
        }
        return -1;
    }

    unsigned char N = packet[1];
    if (N > alInfo.seqNum)
    {
        if (DEBUG_MSGS)
        {
            printf("processDPacket Ns - %x, Nu - %x, seqNums - %x, seqNumu - %x\n", (char) N, (unsigned char) N,
                   (char) alInfo.seqNum, (unsigned char) alInfo.seqNum);
            printf("processDPacket Bad2.\n");
        }
        return -1;
    }
    else if (N < alInfo.seqNum)
    {
        return -2;
    }

    unsigned char L2 = packet[2];
    unsigned char L1 = packet[3];
    *dataSize = L2 * MAX_CHAR + L1;

    *data = (char *) malloc(*dataSize);
    if (*data == NULL)
    {
        if (DEBUG_MSGS)
        {
            printf("processDPacket Bad3.\n");
        }
        return -1;
    }

    memcpy(*data, &packet[4], *dataSize);
    if (DEBUG_MSGS)
    {
        printf("processDPacket success.\n");
    }

    return 0;
}

int processCPacket(char *packet, PacketType *type)
{
    unsigned int index = 0;
    char typeNumber = packet[index];
    if (typeNumber == PACKET_TYPE_END)
    {
        *type = PKT_END;
        return 0;
    }
    else if (typeNumber != PACKET_TYPE_START)
    {
        return -1;
    }
    *type = PKT_START;
    index++;


    unsigned int i;
    const unsigned int N_PARAMS = 2;
    for (i = 0; i < N_PARAMS; i++)
    {
        char T = packet[index];
        index++;
        unsigned char L = (unsigned char) packet[index];
        index++;

        switch (T)
        {
            case FILE_SIZE_TYPE:
                memcpy(&alInfo.fileSize, &packet[index], sizeof(char) * L);
                break;
            case FILE_NAME_TYPE:
                memcpy(alInfo.fileName, &packet[index], L);
                alInfo.fileName[L] = '\0';
                break;
            default:
                return -1;
        }
        index += L;
    }

    return 0;
}

//Building a data packet  
char *buildDPacket(unsigned int dataSize, char *data)
{
    char *packet = (char *) malloc(sizeof(char) * (dataSize + D_PACKET_BASE_SIZE));
    if (packet == NULL)
    {
        return NULL;
    }

    packet[0] = PACKET_TYPE_DATA;
    packet[1] = alInfo.seqNum;
    packet[2] = (char) (dataSize / MAX_CHAR);
    packet[3] = (char) (dataSize % MAX_CHAR);

    unsigned int i;
    for (i = 0; i < dataSize; i++)
    {
        packet[D_PACKET_BASE_SIZE + i] = data[i];
    }

    return packet;
}

//Building a control packet 
char *buildCPacket(PacketType type)
{
    const unsigned char fileNameSize = (unsigned char) strlen(alInfo.fileName);

    char *packet = (char *) malloc(FILE_SIZE_SIZE + fileNameSize + C_PACKET_BASE_SIZE);
    if (packet == NULL)
    {
        return NULL;
    }

    unsigned int index = 0;
    //Get the control field
    if (type == PKT_START)
    {
        packet[index] = PACKET_TYPE_START;
    }
    else if (type == PKT_END)
    {
        packet[index] = PACKET_TYPE_END;
    }
    else
    {
        return NULL;
    }
    index++;

    //Build the first TLV - file size
    packet[index] = FILE_SIZE_TYPE;
    index++;
    packet[index] = (unsigned char) FILE_SIZE_SIZE;
    index++;

    *((unsigned int *) (&packet[index])) = alInfo.fileSize;

    index += sizeof(unsigned int);

    //Build the second TLV - file name
    packet[index] = FILE_NAME_TYPE;
    index++;
    packet[index] = fileNameSize;
    index++;

    unsigned int i;
    for (i = 0; i < fileNameSize; i++)
    {
        packet[index] = alInfo.fileName[i];
        index++;
    }

    return packet;
}
