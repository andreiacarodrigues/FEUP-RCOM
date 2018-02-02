#include <stdlib.h>     //malloc
#include <string.h>     //strncpy
#include <stdbool.h>    //bool

#include <sys/types.h>  //size_t ssize_t

#include <sys/types.h> //open
#include <sys/stat.h> //open
#include <fcntl.h> //open

#include <unistd.h> //tcsetattr

#include <stdio.h> //printf
#include <time.h> //time

#include <errno.h> //errno
#include <datalink.h>

#include "datalink.h"
#include "alarm.h"
#include "util_interface.h"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define RECEIVER_TRIES 10
#define V_TIM_READ 100

static LinkLayerInfo llInfo;
static LinkLayerStats llStats;

ssize_t readFromFd(int fd, char *buf)
{
    volatile int STOP = 0;
    char c;
    size_t index = 0; //index for the next position in message to store a char

    while (STOP == 0)
    {
        ssize_t n = read(fd, &c, 1); //number of bytes read
        if (n == -1)
        {
            perror("write");
            return -1;
        }

        if (c == '\0')
        {
            STOP = 1;
        }

        buf[index] = c;

        index++;
    }

    return index;
}

//Services provided to the application layer
int llopen(int port, ConnectionMode mode)
{
    resetLlStats();

    if (DEBUG_MSGS)
    {
        printf("llopen called for port %d and mode %d\n", port, mode);
    }

    //Set the connection mode
    llInfo.mode = mode;

    //Set the serial port
    if (setPort(port) == -1)
    {
        if (DEBUG_MSGS)
        {
            printf("Error on set port.\n");
        }
        return -1;
    }
    if (DEBUG_MSGS)
    {
        printf("setPort: success.\n");
    }

    //Open the serial port
    int fd = open(llInfo.port, O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }
    if (DEBUG_MSGS)
    {
        printf("llopen- open: success.\n");
    }

    //Set terminal connection settings
    if (setTermios(fd) == -1)
    {
        if (DEBUG_MSGS)
        {
            printf("llopen: setTermios failed.\n");
        }
        return -1;
    }
    if (DEBUG_MSGS)
    {
        printf("llopen- setTermios: success.\n");
    }

    int numTries = 0;
    int isConnected = 0;

    switch (mode)
    {
        case TRANSMIT:
            //Loop to send SET and receive UA
            if (sendSUFrame(fd, SET) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llopen: Could not transmit SET frame. numTries = %d.\n", numTries);
                }
                close(fd);
                return -1;
            }
            llStats.SETFramesSent++;
            numTries++;
            startAlarm(llInfo.timeout);
            while (!isConnected)
            {
                if (alarmFlag)
                {
                    llStats.timeouts++;
                    alarmFlag = 0;
                    if (numTries >= llInfo.maxTries)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llopen: Exceeded max number of tries to send SET frame.\n");
                        }
                        stopAlarm();
                        close(fd);
                        return -1;
                    }
                    if (sendSUFrame(fd, SET) == -1)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llopen: Could not transmit SET frame. numTries = %d.\n", numTries);
                        }
                        close(fd);
                        return -1;
                    }
                    llStats.SETFramesSent++;
                    numTries++;
                }

                ReceivedFrameInfo rInfo;
                if (receiveFrame(fd, &rInfo) == -1)
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llopen: Error while trying to receive UA frame.\n");
                    }
                    close(fd);
                    return -1;
                }

                if (rInfo.type == UA)
                {
                    llStats.UAFramesReceived++;
                    stopAlarm();
                    isConnected = 1;
                }
            }
            //Loop to send SET and receive UA
            if (DEBUG_MSGS)
            {
                printf("llopen: SET frame was sucessfully sent and UA was received!\n");
            }
            break;
        case RECEIVE:
            //Loop to receive SET
            while (!isConnected)
            {
                ReceivedFrameInfo rInfo;
                if (receiveFrame(fd, &rInfo) == -1)
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llopen: Error while trying to receive SET frame.\n");
                    }
                    close(fd);
                    return -1;
                }

                if (rInfo.type == SET)
                {
                    llStats.SETFramesReceived++;
                    if (DEBUG_MSGS)
                    {
                        printf("llopen: SET frame received.\n");
                    }
                    isConnected = 1;
                }
                else
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llopen: Not SET frame received.\n");
                    }
                }
            }
            //End of loop to receive SET
            if (DEBUG_MSGS)
            {
                printf("llopen: SET was received!\n");
            }
            //Send UA frame
            if (sendSUFrame(fd, UA) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llopen: Error while trying to transmit UA frame.\n");
                }
                close(fd);
                return -1;
            }
            llStats.UAFramesReceived++;
            if (DEBUG_MSGS)
            {
                printf("llopen: UA was sent!\n");
            }
            break;
    }

    if (DEBUG_MSGS)
    {
        printf("llopen: Success!\n");
    }

    return fd;
}

int llwrite(int fd, char *buffer, size_t length)
{
    if (DEBUG_MSGS)
    {
        printf("llwrite called\n");
    }

    int numTries = 0;
    int isTransferring = 1;

    if (sendIFrame(fd, buffer, length) == -1)
    {
        if (DEBUG_MSGS)
        {
            printf("llwrite: Error while trying to transmit I frame.\n");
        }
        return -1;
    }
    llStats.IFramesSent++;

    if (DEBUG_MSGS)
    {
        printf("llwrite: after first send\n");
    }

    numTries++;
    startAlarm(llInfo.timeout);

    while (isTransferring)
    {
        // If it's the first try or the alrm went off
        if (alarmFlag)
        {
            llStats.timeouts++;
            alarmFlag = 0;

            if (DEBUG_MSGS)
            {
                printf("llwrite: timeout.\n");
            }
            // If he exceeded the max number of tries
            if (numTries >= llInfo.maxTries)
            {
                stopAlarm();
                printf("Maximum number of retries (%d) exceeded.\n", llInfo.maxTries);
                return -1;
            }

            if (sendIFrame(fd, buffer, length) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llwrite: Error while trying to transmit I frame.\n");
                }
                return -1;
            }
            llStats.IFramesSent++;
            numTries++;
        }

        // Receive the message
        ReceivedFrameInfo rInfo;
        rInfo.type = BAD;
        rInfo.error = EMPTY;
        if (receiveFrame(fd, &rInfo) == -1)
        {
            return -1;
        }

        if (rInfo.type == RR)
        {
            llStats.RRFramesReceived++;
            if (DEBUG_MSGS)
            {
                printf("llwrite: I frame was sent and RR was received!\n");
            }
            if (llInfo.seqNum != rInfo.seqNum)
            {
                if (DEBUG_MSGS)
                {
                    printf("llwrite: RRseqNum - %x.\n", (unsigned char) rInfo.seqNum);
                }
                llInfo.seqNum = rInfo.seqNum;
            }

            stopAlarm();
            isTransferring = 0;
        }
        else if (rInfo.type == REJ) //resend the frame
        {
            llStats.REJFramesReceived++;
            if (DEBUG_MSGS)
            {
                printf("llwrite: I frame was sent and REJ was received.\n");
            }
            stopAlarm();
            if (sendIFrame(fd, buffer, length) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llwrite: Error while trying to transmit I frame.\n");
                }
                return -1;
            }
            llStats.IFramesSent++;
            startAlarm(llInfo.timeout);
            numTries = 1;
        }
    }

    if (DEBUG_MSGS)
    {
        printf("llwrite: success.\n");
    }
    return 0;
}

ssize_t llread(int fd, char **buffer)
{
    ReceivedFrameInfo msg;

    msg.type = BAD;
    msg.error = EMPTY;

    unsigned int tries = 0;

    int isReceiving = 1;
    while (isReceiving)
    {
        if(DEBUG_MSGS)
        {
            printf("llread: tries - %u\n", tries);
        }
        if (tries >= RECEIVER_TRIES)
        {
            if (DEBUG_MSGS)
            {
                printf("llread: max tries reached.\n");
            }
            return -1;
        }

        int return_value = receiveFrame(fd, &msg);
        if (return_value == -1)
        {
            if (DEBUG_MSGS)
            {
                printf("llread: error while trying to receive I frame.\n");
            }
            return -1;
        }
        else if(return_value == -2)
        {
            tries++;
        }

        if (DEBUG_MSGS)
        {
            printf("llread: testing type.\n");
        }

        switch (msg.type)
        {
            case BAD:
                if (msg.error == BAD_BCC2)
                {
                    llInfo.seqNum = msg.seqNum;
                    if (sendSUFrame(fd, REJ) == -1)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llread: I frame had BCC2 error, failed to send REJ frame.\n");
                        }
                        return -1;
                    }
                    llStats.REJFramesSent++;
                    if (DEBUG_MSGS)
                    {
                        printf("llread: I frame had BCC2 error, sent REJ frame successfully.\n");
                    }
                }
                else if (msg.error == BAD_SEQ_NUM)
                {
                    if (sendSUFrame(fd, RR) == -1)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llread: I frame had SEQ_NUM error, failed to send RR frame.\n");
                        }
                        return -1;
                    }
                    if (DEBUG_MSGS)
                    {
                        printf("llread: I frame had SEQ_NUM error, sent RR frame successfully.\n");
                    }
                }
                break;
            case DISC:
                llStats.DISCFramesReceived++;
                if (DEBUG_MSGS)
                {
                    printf("llread: received disconnected frame.\n");
                }
                return -1;
            case INFO:
                llStats.IFramesReceived++;
                if (DEBUG_MSGS)
                {
                    printf("llread: received info frame.\n");
                }
                if (llInfo.seqNum == msg.seqNum)
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llread: passed sequence number test.\n");
                    }
                    *buffer = malloc(msg.packetSize);
                    memcpy(*buffer, msg.packet, msg.packetSize);
                    free(msg.packet);

                    if (DEBUG_MSGS)
                    {
                        printf("llread: seqNum - %x\n", llInfo.seqNum);
                    }
                    llInfo.seqNum = ((~msg.seqNum) & (unsigned char) 0x1);
                    if (DEBUG_MSGS)
                    {
                        printf("llread: seqNum - %x\n", llInfo.seqNum);
                    }
                    if (sendSUFrame(fd, RR) == -1)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llread: received correct I frame, was not able to send RR.\n");
                        }
                        return -1;
                    }
                    llStats.RRFramesSent++;
                    if (DEBUG_MSGS)
                    {
                        printf("llread: received correct I frame, sent RR successfuly.\n");
                    }

                    isReceiving = 0;

                }
                break;
            default:
                if (DEBUG_MSGS)
                {
                    printf("llread: received another frame type.\n");
                }
                break;
        }
    }

    if (DEBUG_MSGS)
    {
        printf("llread: success.\n");
    }

    return msg.packetSize;
}

int llclose(int fd)
{
    if (DEBUG_MSGS)
    {
        printf("llclose was called.\n");
    }

    int numTries = 0;
    int isDisconnected = 0;
    int receivedUA = 0;

    switch (llInfo.mode)
    {
        case TRANSMIT:
            //Loop to send DISC frame and receive DISC frame
            if (sendSUFrame(fd, DISC) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llclose: Could not transmit DISC frame. numTries = %d.\n", numTries);
                }
                tcflush(fd, TCIOFLUSH);
                close(fd);
                return -1;
            }
            llStats.DISCFramesSent++;
            numTries++;
            startAlarm(llInfo.timeout);
            while (!isDisconnected)
            {
                if (alarmFlag)
                {
                    llStats.timeouts++;
                    alarmFlag = 0;

                    if (numTries >= llInfo.maxTries)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llclose: Exceeded max number of tries to send DISC frame.\n");
                        }
                        stopAlarm();
                        tcflush(fd, TCIOFLUSH);
                        close(fd);
                        return (-1);
                    }

                    if (sendSUFrame(fd, DISC) == -1)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llclose: Could not transmit DISC frame. numTries = %d.\n", numTries);
                        }
                        stopAlarm();
                        tcflush(fd, TCIOFLUSH);
                        close(fd);
                        return -1;
                    }
                    llStats.DISCFramesSent++;
                    numTries++;
                }

                ReceivedFrameInfo rInfo;
                if (receiveFrame(fd, &rInfo) == -1)
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llclose: Error while trying to receive DISC frame.\n");
                    }
                    stopAlarm();
                    tcflush(fd, TCIOFLUSH);
                    close(fd);
                    return -1;
                }
                if (rInfo.type == DISC)
                {
                    llStats.DISCFramesReceived++;
                    stopAlarm();
                    isDisconnected = 1;
                }
            }
            //End of loop to send DISC frame and receive DISC frame
            if (DEBUG_MSGS)
            {
                printf("llclose: DISC and sent was DISC was received!\n");
            }
            //Send UA frame
            if (sendSUFrame(fd, UA) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llclose: Could not transmit UA frame.\n");
                }
                tcflush(fd, TCIOFLUSH);
                close(fd);
                return -1;
            }
            llStats.UAFramesSent++;
            if (DEBUG_MSGS)
            {
                printf("llclose: UA was sent!\n");
            }
            break;
        case RECEIVE:
            //Loop to receive DISC
            while (!isDisconnected)
            {
                ReceivedFrameInfo rInfo;
                if (receiveFrame(fd, &rInfo) == -1)
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llclose: Error while trying to receive DISC frame.\n");
                    }
                    tcflush(fd, TCIOFLUSH);
                    close(fd);
                    return -1;
                }

                if (rInfo.type == DISC)
                {
                    llStats.DISCFramesReceived++;
                    isDisconnected = true;
                }
            }
            //End of loop to receive DISC
            if (DEBUG_MSGS)
            {
                printf("llclose: DISC was received!\n");
            }

            //Loop to send DISC and receive UA
            if (sendSUFrame(fd, DISC) == -1)
            {
                if (DEBUG_MSGS)
                {
                    printf("llclose: Could not transmit DISC frame. numTries = %d.\n", numTries);
                }
                tcflush(fd, TCIOFLUSH);
                close(fd);
                return -1;
            }
            llStats.DISCFramesSent++;
            numTries++;
            startAlarm(llInfo.timeout);
            while (!receivedUA)
            {
                if (alarmFlag)
                {
                    alarmFlag = 0;

                    if (numTries >= llInfo.maxTries)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llclose: Exceeded max number of tries to send DISC frame.\n");
                        }
                        stopAlarm();
                        tcflush(fd, TCIOFLUSH);
                        close(fd);
                        return -1;
                    }

                    if (sendSUFrame(fd, DISC) == -1)
                    {
                        if (DEBUG_MSGS)
                        {
                            printf("llclose: Could not transmit DISC frame. numTries = %d.\n", numTries);
                        }
                        stopAlarm();
                        tcflush(fd, TCIOFLUSH);
                        close(fd);
                        return -1;
                    }
                    llStats.DISCFramesSent++;
                    numTries++;
                }

                ReceivedFrameInfo rInfo;
                if (receiveFrame(fd, &rInfo) == -1)
                {
                    if (DEBUG_MSGS)
                    {
                        printf("llclose: Error while trying to receive UA frame.\n");
                    }
                    stopAlarm();
                    tcflush(fd, TCIOFLUSH);
                    close(fd);
                    return -1;
                }
                if (rInfo.type == UA)
                {
                    llStats.UAFramesReceived++;
                    stopAlarm();
                    receivedUA = 1;
                }
            }
            //End of loop to send DISC frame and receive UA frame
            if (DEBUG_MSGS)
            {
                printf("llclose: DISC and sent was UA was received!\n");
            }
            break;
        default:
            break;
    }

    tcflush(fd, TCIOFLUSH);
    close(fd);
    return 0;
}

//Building the ll_info
int setPort(int port)
{
    switch (port)
    {
        case COM1:
            strncpy(llInfo.port, COM1_PORT_NAME, PORT_NAME_SIZE);
            break;
        case COM2:
            strncpy(llInfo.port, COM2_PORT_NAME, PORT_NAME_SIZE);
            break;
        case COM3:
            strncpy(llInfo.port, COM3_PORT_NAME, PORT_NAME_SIZE);
            break;
        default:
            return -1;
    }
    return 0;
}

int setTermios(int fd)
{
    struct termios newtio;
    bzero(&newtio, sizeof(newtio)); //fill newtio with 0's
    // CS8: 8n1 (8 bits, sem bit de paridade,1 stopbit)
    // CLOCAL: ligação local, sem modem
    // CREAD: activa a recepção de caracteres
    newtio.c_cflag = llInfo.baudrate | CS8 | CLOCAL | CREAD;
    // IGNPAR: Ignora erros de paridade
    newtio.c_iflag = IGNPAR;
    //Saida nao processada
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = V_TIM_READ;
    newtio.c_cc[VMIN] = 0;


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    if (DEBUG_MSGS)
    {
        printf("New termios structure set.\n");
    }

    return 0;
}

int initDatalinkLayer(tcflag_t baudrate, size_t frameSize, unsigned int timeout, unsigned int maxTries,
                      double byteErrorRatio)
{
    srand(time(NULL));
    llInfo.baudrate = baudrate;
    llInfo.frameSize = frameSize;
    llInfo.seqNum = 0;
    llInfo.timeout = timeout;
    llInfo.maxTries = maxTries;
    llInfo.byteErrorRatio = byteErrorRatio;

    return 0;
}

//Stats
void resetLlStats()
{
    bzero(&llStats, sizeof(llStats));
}

void printLlXmitStats()
{
    printf(" --- Link Layer Stats ---\n");

    printf(YEL "I Frames Sent/Received: ");
    printf(CYN "%u/%u.\n", llStats.IFramesSent, llStats.IFramesReceived);

    printf(YEL "SET Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.SETFramesSent, llStats.SETFramesReceived);

    printf(YEL "DISC Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.DISCFramesSent, llStats.DISCFramesReceived);

    printf(YEL "UA Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.UAFramesSent, llStats.UAFramesReceived);

    printf(YEL "RR Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.RRFramesSent, llStats.RRFramesReceived);

    printf(YEL "REJ Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.REJFramesSent, llStats.REJFramesReceived);

    printf(YEL "Number of timeouts: ");
    printf(CYN "%u\n" RESET, llStats.timeouts);
}

void printLlRcvrStats()
{
    printf(" --- Link Layer Stats ---\n");

    printf(YEL "I Frames Sent/Received: ");
    printf(CYN "%u/%u.\n", llStats.IFramesSent, llStats.IFramesReceived);

    printf(YEL "SET Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.SETFramesSent, llStats.SETFramesReceived);

    printf(YEL "DISC Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.DISCFramesSent, llStats.DISCFramesReceived);

    printf(YEL "UA Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.UAFramesSent, llStats.UAFramesReceived);

    printf(YEL "RR Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.RRFramesSent, llStats.RRFramesReceived);

    printf(YEL "REJ Frames Sent/Received: ");
    printf(CYN "%u/%u\n", llStats.REJFramesSent, llStats.REJFramesReceived);

    printf(YEL "badSize: ");
    printf(CYN "%u\n", llStats.badSize);
    printf(YEL "badA: ");
    printf(CYN "%u\n", llStats.badA);
    printf(YEL "badC: ");
    printf(CYN "%u\n", llStats.badC);
    printf(YEL "badBCC1: ");
    printf(CYN "%u\n", llStats.badBCC1);
    printf(YEL "badBCC2: ");
    printf(CYN "%u\n" RESET, llStats.badBCC2);
}

//Sending frames
int sendSUFrame(int fd, FrameType type)
{
    if (DEBUG_MSGS)
    {
        printf("sendSUFrame called.\n");
    }

    char *frame = buildSUFrame(type);
    if (DEBUG_MSGS)
    {
        printByteByByte(frame, 5);
        printf("\n");
    }

    stuff(&frame, SU_FRAME_SIZE);
    ssize_t nSent;
    if ((nSent = write(fd, frame, SU_FRAME_SIZE)) == -1)
    {
        perror("write");
        return -1;
    }
    if (DEBUG_MSGS)
    {
        printf("nSent = %zd \n", nSent);
    }

    free(frame);

    if (DEBUG_MSGS)
    {
        printf("sendSUFrame: Success!\n");
    }

    return 0;
}

int sendIFrame(int fd, char *data, size_t length)
{
    if ((length + 6) > llInfo.frameSize)
    {
        if (DEBUG_MSGS)
        {
            printf("sendIFrame: Bad frame size!\n");
        }
        return -1;
    }

    char *frame = buildIFrame(data, length);

    unsigned int frame_size = I_FRAME_BASE_SIZE + length;

    unsigned int new_frame_size = stuff(&frame, frame_size);

    if (DEBUG_MSGS)
    {
        printByteByByte(frame, new_frame_size);
        printf("\n");
    }

    int nWritten = write(fd, frame, new_frame_size);

    if (DEBUG_MSGS)
    {
        printf("sendIFrame: nWritten - %d\n", nWritten);
    }

    free(frame);

    return nWritten;
}

//Receiving frames
int receiveFrame(int fd, ReceivedFrameInfo *rInfo)
{
    const size_t buffer_size = llInfo.frameSize * 2;
    char *buf = malloc(buffer_size);   // Buffer to store read data

    bool readFirstFlag = 0;
    bool readSecondFlag = 0;
    unsigned int bufIndex = 0;
    static bool twoSizeFrame = 0;
    if (twoSizeFrame == 1)
    {
        readFirstFlag = 1;
        buf[bufIndex] = FLAG;
        bufIndex++;
    }

    if (DEBUG_MSGS)
    {
        printf("receiveFrame: AA.\n");
    }

    while (!readSecondFlag)
    {
        if (DEBUG_MSGS)
        {
            printf("receiveFrame: BB.\n");
        }
        char c;
        if (read(fd, &c, 1) <= 0)
        {
            if (DEBUG_MSGS)
            {
                printf("receiveFrame: CC.\n");
            }
            if (errno != EINTR)
            {
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: DD.\n");
                }
                return -1;
            }
            else
            {
                return -2;
            }
        }
        if (DEBUG_MSGS)
        {
            printf("receiveFrame: char read %c.\n", c);
        }
        if (!readFirstFlag)
        {
            if (DEBUG_MSGS)
            {
                printf("receiveFrame: EE.\n");
            }
            if (c == FLAG)
            {
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: FF.\n");
                }
                readFirstFlag = 1;
                buf[bufIndex] = c;
                bufIndex++;
            }
        }
        else
        {
            if (DEBUG_MSGS)
            {
                printf("receiveFrame: GG.\n");
            }
            buf[bufIndex] = c;
            bufIndex++;
            if (c == FLAG)
            {
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: HH.\n");
                }
                readSecondFlag = 1;
            }
        }
    }
    if (DEBUG_MSGS)
    {
        printf("receiveFrame: II.\n");
    }
    insertRandomErrors(buf, (size_t) bufIndex);

    unsigned int size = destuff(&buf, bufIndex);
    if (DEBUG_MSGS)
    {
        printf("receiveFrame: post destuff.\n");
        printByteByByte(buf, size);
        printf("\n");
    }
    if (processFrame(buf, size, rInfo) == -1) // Process frame data found
    {
        if (DEBUG_MSGS)
        {
            printf("receiveFrame: processFrame failed.\n");
        }
        return -1;
    }
    twoSizeFrame = 0;
    if (rInfo->type == BAD)
    {
        if (rInfo->error == BAD_SIZE_TWO)
        {
            twoSizeFrame = 1;
        }
    }

    if (DEBUG_MSGS)
    {
        printf("receiveFrame: JJ.\n");
    }

    return 0;
}

/*
//Receiving frames
int receiveFrame(int fd, ReceivedFrameInfo *rInfo)
{
    if (DEBUG_MSGS)
    {
        printf("receiveFrame called.\n");
    }

    const size_t buffer_size = llInfo.frameSize*2;
    char *buf = malloc(buffer_size);   // Buffer to store read data

    bool hasFirstFlag = false;  // Found first flag?
    size_t bytesAhead = 0;      // Bytes left to process

    size_t firstFlagIndex;
    size_t secondFlagIndex;

    if(DEBUG_MSGS)
	{
		printf("receiveFrame: AA \n");	
	}
    while (true)
    {
		if(DEBUG_MSGS)
		{
			 printf("\n");
       		 printf("receiveFrame: BB \n");
		}
        ssize_t n; // Read data
        if(DEBUG_MSGS) printf("receiveFrame: before read \n");
        n = read(fd, buf + bytesAhead, buffer_size - bytesAhead); // Read data from serial port
        if(DEBUG_MSGS) printf("receiveFrame: %lu read .\n", n);
        if (n <= 0) // If the serial port is empty (no more data)
        {
            if (-1 == n)
            {
                if (errno != EINTR)
                {
                    perror("read");
                    return -1;
                }
                return -2;
            }
            else if (0 == n)
            {
                return -2;
            }
        }
        else
        {
            insertRandomErrors(buf + bytesAhead, (size_t)n);
            bytesAhead += n;
        }

        if (DEBUG_MSGS)
        {
            printf("receiveFrame: %lu bytes ahead\n", bytesAhead);
        }

        if (!hasFirstFlag)
        {
            for (firstFlagIndex = 0; firstFlagIndex < bytesAhead; firstFlagIndex++) // For each byte in the buffer...
            {
                if (FLAG == buf[firstFlagIndex]) // If the byte value is the same as the FLAG value...
                {
                    if (DEBUG_MSGS)
                    {
                        printf("receiveFrame: first FLAG found.\n");
                    }
                    hasFirstFlag = true;
                    break; // Stop scanning, first FLAG found
                }
            }

            if (hasFirstFlag) // If the first flag was found within the buffer...
            {
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: first FLAG found confirmed.\n");
                }
                if (0 != firstFlagIndex) // If the flag is not already at the start of the buffer...
                {
                    if (DEBUG_MSGS)
                    {
                        printf("receiveFrame: before memmove.\n");
                    }
                    memmove(buf, buf + firstFlagIndex, bytesAhead - firstFlagIndex); // Shift the flag to the start
                    if (DEBUG_MSGS)
                    {
                        printf("receiveFrame: after memmove.\n");
                    }
                    bytesAhead -= firstFlagIndex; // Change bytesAhead to the amount of bytes read from the flag (including) forward
                }
            }
        }

        if (hasFirstFlag && (bytesAhead > 1)) // Found first flag AND has usable data
        {
            // BLOCK: Scan read data for a starting flag
            // NOTE: buf[0] is the first FLAG value found
            for (secondFlagIndex = 1; secondFlagIndex < bytesAhead; secondFlagIndex++) // For each byte in the buffer...
            {
                if (buf[secondFlagIndex] == FLAG) // If the byte value is the same as the FLAG value...
                {
                    break; // Stop scanning, second FLAG found
                }
            }
            // secondFlagIndex persists downwards
            // END BLOCK

            if (secondFlagIndex < bytesAhead) // If the second flag was found within the buffer...
            {
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: pre destuff.\n");
                }
                unsigned int size = destuff(&buf, secondFlagIndex + 1);
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: post destuff.\n");
                    printByteByByte(buf, size);
                    printf("\n");
                }
                if (processFrame(buf, size, rInfo) == -1) // Process frame data found
                {
                    if (DEBUG_MSGS)
                    {
                        printf("receiveFrame: processFrame failed.\n");
                    }
                    return -1;
                }
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: type - %d.\n", rInfo->type);
                }
                if (DEBUG_MSGS)
                {
                    printf("receiveFrame: error - %d.\n", rInfo->error);
                }
                return 0;
            }
        }
    }
}
*/

int processFrame(char *data, size_t dataSize, ReceivedFrameInfo *rInfo)
{
    if (DEBUG_MSGS)
    {
        printf("called processframe.\n");
    }

    ReceivedFrameInfo receiveInfo;

    if (dataSize < 5)
    {
        if (DEBUG_MSGS)
        {
            printf("processFrame: BAD_SIZE_UNDER_5.\n");
        }
        receiveInfo.type = BAD;
        receiveInfo.error = BAD_SIZE;
        if (dataSize == 2)
        {
            receiveInfo.error = BAD_SIZE_TWO;
        }
        *rInfo = receiveInfo;
        return 0;
    }

    char A = data[1];
    char C = data[2];
    char BCC1 = data[3];

    if (BCC1 != (A ^ C))
    {
        if (DEBUG_MSGS)
        {
            printf("processFrame: BAD_BCC1.\n");
        }
        receiveInfo.type = BAD;
        receiveInfo.error = BAD_BCC1;
        *rInfo = receiveInfo;
        llStats.badA++;
        return 0;
    }

    if ((0x03 != A) && (0x01 != A))
    {
        if (DEBUG_MSGS)
        {
            printf("processFrame: BAD_A.\n");
        }
        receiveInfo.type = BAD;
        receiveInfo.error = BAD_A;
        *rInfo = receiveInfo;
        llStats.badSize++;
        return 0;
    }

    if (5 == dataSize)
    {
        unsigned char C = (unsigned char) data[2];
        if (SET_CF == C)
        {
            receiveInfo.type = SET;
        }
        else if (DISC_CF == C)
        {
            receiveInfo.type = DISC;
        }
        else if (UA_CF == C)
        {
            receiveInfo.type = UA;
        }
        else if (RR_BASE_CF == (C & 0x7f))
        {
            receiveInfo.seqNum = (C >> 7) & (unsigned char) 0x1;
            receiveInfo.type = RR;
        }
        else if (REJ_BASE_CF == (C & 0x7f))
        {
            receiveInfo.seqNum = (C >> 7) & (unsigned char) 0x1;
            receiveInfo.type = REJ;
        }
        else
        {
            if (DEBUG_MSGS)
            {
                printf("processFrame: BAD_C_5_FRAME_SIZE.\n");
            }
            receiveInfo.type = BAD;
            receiveInfo.error = BAD_C;
            *rInfo = receiveInfo;
            llStats.badC++;
            return 0;
        }
    }
    else
    {
        if (6 == dataSize)
        {
            if (DEBUG_MSGS)
            {
                printf("processFrame: BAD_SIZE_6_AND_INFO.\n");
            }
            receiveInfo.type = BAD;
            receiveInfo.error = BAD_SIZE;
            *rInfo = receiveInfo;
            return 0;
        }

        if (INFO_BASE_CF == (C & 0xbf))
        {
            receiveInfo.packet = &data[4];
            receiveInfo.packetSize = dataSize - 4 - 2;
            receiveInfo.seqNum = (unsigned char) ((C & 0x40) >> 6);
            receiveInfo.type = INFO;
        }
        else
        {
            if (DEBUG_MSGS)
            {
                printf("processFrame: Bad3.\n");
            }
            receiveInfo.type = BAD;
            receiveInfo.error = BAD_C;
            *rInfo = receiveInfo;
            llStats.badC++;
            return 0;
        }
    }

    if (RR == receiveInfo.type)
    {
        if (receiveInfo.seqNum == llInfo.seqNum)
        {
            receiveInfo.type = BAD;
            receiveInfo.error = BAD_C;
            if (DEBUG_MSGS)
            {
                printf("processFrame: BAD_C_RR.\n");
            }
            *rInfo = receiveInfo;
            return 0;
        }
    }
    else if ((REJ == receiveInfo.type) || (INFO == receiveInfo.type))
    {
        if (receiveInfo.seqNum != llInfo.seqNum)
        {
            receiveInfo.type = BAD;
            if (REJ == receiveInfo.type)
            {
                receiveInfo.error = BAD_C;
                if (DEBUG_MSGS)
                {
                    printf("processFrame: BAD_C_REJ.\n");
                }
                *rInfo = receiveInfo;
                return 0;
            }
            else
            {
                receiveInfo.error = BAD_SEQ_NUM;
                if (DEBUG_MSGS)
                {
                    printf("processFrame: BAD_SEQ_NUM.\n");
                }
                *rInfo = receiveInfo;
                return 0;
            }
        }
    }

    if (A == getAddressField(receiveInfo.type))
    {
        if (DEBUG_MSGS)
        {
            printf("processFrame: Bad5.\n");
        }
        receiveInfo.type = BAD;
        receiveInfo.error = BAD_A;
        *rInfo = receiveInfo;
        llStats.badA++;
        return 0;
    }

    if (INFO == receiveInfo.type)
    {
        char BCC2 = 0;
        size_t index;
        for (index = 4; index < (dataSize - 2); index++)
        {
            BCC2 ^= data[index];
        }
        if (BCC2 != data[dataSize - 2])
        {
            if (DEBUG_MSGS)
            {
                printf("processFrame: BAD_BCC2.\n");
            }
            receiveInfo.type = BAD;
            receiveInfo.error = BAD_BCC2;
            *rInfo = receiveInfo;
            llStats.badBCC2++;
            return 0;
        }
        size_t packetSize = (dataSize - 2) - 4;
        receiveInfo.packet = malloc(packetSize);
        memcpy(receiveInfo.packet, &data[4], packetSize);
        receiveInfo.packetSize = packetSize;
        if (DEBUG_MSGS)
        {
            printf("processFrame: packetSize-%lu\n", packetSize);
        }
        free(data);
    }

    *rInfo = receiveInfo;

    return 0;
}

//Building information frames (I frames)
char *buildIFrame(char *data, unsigned int length)
{
    char *frame = NULL;

    frame = (char *) malloc(I_FRAME_BASE_SIZE + length);
    if (frame == NULL)
    {
        return frame;
    }

    frame[0] = FLAG;
    frame[1] = getAddressField(INFO);
    frame[2] = getControlField(INFO);
    frame[3] = (frame[1] ^ frame[2]); //xor operation returns 1 if the number of 1 bits are odd
    frame[4 + length] = 0;
    unsigned int i;
    for (i = 0; i < length; i++)
    {
        frame[4 + i] = data[i];
        frame[4 + length] ^= frame[4 + i];
    }
    frame[5 + length] = FLAG;

    return frame;
}

//Building non-information frames (S and U frames)
char *buildSUFrame(FrameType type)
{
    char *frame = NULL;
    if (type != INFO)
    {
        frame = (char *) malloc(SU_FRAME_SIZE);
        if (frame == NULL)
        {
            return frame;
        }

        frame[0] = FLAG;
        frame[1] = getAddressField(type);
        frame[2] = getControlField(type);
        frame[3] = (frame[1] ^ frame[2]); //xor operation returns 1 if the number of 1 bits are odd
        frame[4] = FLAG;
    }

    return frame;
}

FrameDesignation getFrameDesignation(FrameType type)
{
    switch (type)
    {
        case INFO:
            return FD_CMD;
        case SET:
            return FD_CMD;
        case DISC:
            return FD_CMD;
        case UA:
            return FD_CMD;
        case RR:
            return FD_REP;
        case REJ:
            return FD_REP;
        default:
            break;
    }
    return -1;
}

char getAddressField(FrameType type)
{
    char af = -1;

    FrameDesignation desig = getFrameDesignation(type);
    if ((desig == FD_CMD) && (llInfo.mode == TRANSMIT))
    {
        af = TRANSMITTER_CMD;
    }
    else if ((desig == FD_REP) && (llInfo.mode == TRANSMIT))
    {
        af = TRANSMITTER_REP;
    }
    else if ((desig == FD_CMD) && (llInfo.mode == RECEIVE))
    {
        af = RECEIVER_CMD;
    }
    else if ((desig == FD_REP) && (llInfo.mode == RECEIVE))
    {
        af = RECEIVER_REP;
    }
    return af;
}

char getControlField(FrameType type)
{
    char cf = -1;
    char seqNumBit;

    switch (type)
    {
        case INFO:
            cf = INFO_BASE_CF;
            seqNumBit = (llInfo.seqNum << 6);
            if (DEBUG_MSGS)
            {
                printf("getControlField: seqNum - %x, seqNumBit - %x.\n", (unsigned char) llInfo.seqNum,
                       (unsigned char) seqNumBit);
            }
            cf |= seqNumBit;
            break;
        case SET:
            cf = SET_CF;
            break;
        case DISC:
            cf = DISC_CF;
            break;
        case UA:
            cf = UA_CF;
            break;
        case RR:
            cf = RR_BASE_CF;
            if (DEBUG_MSGS)
            {
                printf("getControlField: RRseqNum - %x\n", (unsigned char) llInfo.seqNum);
            }
            seqNumBit = (llInfo.seqNum << 7);
            if (DEBUG_MSGS)
            {
                printf("getControlField: RRseqNum - %x, seqNumBit - %x.\n", (unsigned char) llInfo.seqNum,
                       (unsigned char) seqNumBit);
            }
            cf |= seqNumBit;
            if (DEBUG_MSGS)
            {
                printf("getControlField: RRseqNum - %x, seqNumBit - %x, cf - %x.\n", (unsigned char) llInfo.seqNum,
                       (unsigned char) seqNumBit, (unsigned char) cf);
            }
            break;
        case REJ:
            cf = REJ_BASE_CF;
            seqNumBit = (llInfo.seqNum << 7);
            cf |= seqNumBit;
            break;
        default:
            break;
    }
    return cf;
}


//Stuffing/destuffing frames
unsigned int stuff(char **frame, unsigned int frameSize)
{
    //Step 1: Determine the size of the stuffed frame
    unsigned int newFrameSize = frameSize;
    unsigned int i = 1; //start at 1 so we dont stuff the flag !!

    //we also dont stuff the (frameSize - 1)th element because it is a flag !!
    for (i = 1; i < (frameSize - 1); i++)
    {
        char c = (*frame)[i];
        if ((c == FLAG) || (c == ESCAPE))
        {
            newFrameSize++;
        }
    }

    //Step 2: Allocate memory for the new frame
    char *newFrame = (char *) malloc(sizeof(char) * newFrameSize);
    if (newFrame == NULL)
    {
        return 0;
    }

    //Step 3: Store the frame data in the allocate memory
    newFrame[0] = FLAG; //insert the initial FLAG
    unsigned int newFrameIndex = 1;

    for (i = 1; i < (frameSize - 1); i++)
    {
        char c = (*frame)[i];
        if (c == FLAG)
        {
            newFrame[newFrameIndex] = ESCAPE;
            newFrameIndex++;
            newFrame[newFrameIndex] = FLAG_STUFF;
        }
        else if (c == ESCAPE)
        {
            newFrame[newFrameIndex] = ESCAPE;
            newFrameIndex++;
            newFrame[newFrameIndex] = ESCAPE_STUFF;
        }
        else
        {
            newFrame[newFrameIndex] = c;
        }
        newFrameIndex++;
    }

    newFrame[newFrameIndex] = FLAG; //insert the final FLAG

    //Step 4: Free the memory used for the original frame
    free(*frame);
    *frame = newFrame;

    return newFrameSize;
}

unsigned int destuff(char **frame, unsigned int frameSize)
{
    //Step 1: Update the frame contents
    unsigned int i = 0;
    unsigned int newFrameIndex = 0;

    for (i = 0; i < (frameSize - 1); i++)
    {
        char c = (*frame)[i];
        char c2 = (*frame)[i + 1];
        if ((c == ESCAPE) && (c2 == FLAG_STUFF))
        {
            (*frame)[newFrameIndex] = FLAG;
            i++;
        }
        else if ((c == ESCAPE) && (c2 == ESCAPE_STUFF))
        {
            (*frame)[newFrameIndex] = ESCAPE;
            i++;
        }
        else
        {
            (*frame)[newFrameIndex] = c;
        }
        newFrameIndex++;
    }
    (*frame)[newFrameIndex] = (*frame)[frameSize - 1];
    newFrameIndex++;

    //Step 2: Shorten the frame allocated memory
    *frame = (char *) realloc(*frame, sizeof(char) * newFrameIndex);
    if (frame == NULL)
    {
        return 0;
    }

    return newFrameIndex;
}

void insertRandomErrors(char *buffer, size_t length)
{
    unsigned char *bufferToUnsigned = (unsigned char *) buffer;
    size_t i;
    for (i = 0; i < length; i++)
    {
        int randomResult;
        randomResult = rand();
        if (randomResult <= (llInfo.byteErrorRatio * RAND_MAX))
        {
            randomResult = rand() % 256;
            bufferToUnsigned[i] = (unsigned char) randomResult;
        }
    }
}

tcflag_t getBaudrate(int rate)
{
    int noBs[] = NO_BS;
    tcflag_t bs[] = BS;

    unsigned int index;
    for (index = 0; index < sizeof(noBs) / sizeof(tcflag_t); index++)
    {
        if (rate == noBs[index])
        {
            return bs[index];
        }
    }
    return (tcflag_t) -1;
}

void printBaudrates()
{
    int noBs[] = NO_BS;

    unsigned int index;
    for (index = 0; index < sizeof(noBs) / sizeof(tcflag_t); index++)
    {
        printf("%u\n", noBs[index]);
    }
}
