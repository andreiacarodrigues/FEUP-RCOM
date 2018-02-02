#include <stdio.h> //printf
#include <stdlib.h> //strtol
#include <errno.h> //errno

#include "interface.h"
#include "application.h"
#include "datalink.h"
#include "util_interface.h"

#define MAX_INPUT 250

#define DFL_PORT COM1
#define DFL_UIBAUDRATE 9600
#define DFL_FRAMESIZE 256
#define DFL_TIMEOUT 1
#define DFL_MAXTRIES 3
#define DFL_BYTE_ERROR_RATIO 0.0

static CommunicationConfig config;

int startInterface()
{
    printWelcomeMessage();

    int connectionMode = -1;
    int waiting = 1;

    // Chooses the connection mode
    printf("Choose your connection mode: \n 0 - Sender		1 - Receiver");
    while (waiting)
    {
        if (scanf("%d", &connectionMode))
        {
            if ((connectionMode != 0) && (connectionMode != 1))
            {
                perror("Connection mode value incorrect. Insert a valid number.");
            }
            else
            {
                waiting = 0;
            }
        }
    }

    cleanBuffer();

    return connectionMode;
}

void initCommunicationConfig()
{
    config.port = DFL_PORT;
    config.uiBaudrate = DFL_UIBAUDRATE;
    config.baudrate = getBaudrate(config.uiBaudrate);
    config.frameSize = DFL_FRAMESIZE;
    config.timeout = DFL_TIMEOUT;
    config.maxTries = DFL_MAXTRIES;
    config.byteErrorRatio = DFL_BYTE_ERROR_RATIO;
}

//Reading input
int readLongInt(long int *li)
{
    char input[MAX_INPUT];
    if (fgets(input, MAX_INPUT, stdin) == NULL)
    {
        return -1;
    }

    errno = 0;
    /* "the calling  program should set errno to 0 before the call, and then deter‐
       mine if an error occurred by checking whether errno has a nonzero value
       after the call." (man strtol) */
    *li = strtol(input, NULL, 10);
    if (errno != 0)
    {
        return -1;
    }

    return 0;
}

int readDouble(double *d)
{
    char input[MAX_INPUT];
    if (fgets(input, MAX_INPUT, stdin) == NULL)
    {
        return -1;
    }

    errno = 0;
    /* "the calling  program should set errno to 0 before the call, and then deter‐
       mine if an error occurred by checking whether errno has a nonzero value
       after the call." (man strtod) */
    *d = strtod(input, NULL);
    if (errno != 0)
    {
        return -1;
    }

    return 0;
}

void pressEnter()
{
    printf("Press ENTER to continue.");
    int c = getchar();
    if ((c != '\n') && (c != EOF))
    {
        cleanBuffer();
    }
}

//Main menu
void printMainMenu()
{
    printWelcomeMessage();
    printConfig();
    printMainMenuOptions();
}

void printWelcomeMessage()
{
    printf("-------------------------------------\n");
    printf("-  RCOM - Data Connection Protocol  -\n");
    printf("-------------------------------------\n");
}

void printConfig()
{
    printf("Communication parameters:\n");

    printf(YEL "Port: ");
    switch (config.port)
    {
        case COM1:
            printf(CYN "COM1 (ttyS0).\n");
            break;
        case COM2:
            printf(CYN "COM2 (ttyS1).\n");
            break;
        case COM3:
            printf(CYN "COM3 (ttyS2).\n");
            break;
    }

    printf(YEL "Baudrate: ");
    printf(CYN "%ld Bd.\n", config.uiBaudrate);

    printf(YEL "Frame size: ");
    printf(CYN "%zu Bytes.\n", config.frameSize);

    printf(YEL "Timeout: ");
    printf(CYN "%u s.\n", config.timeout);

    printf(YEL "Max number of retransmissions: ");
    printf(CYN "%u.\n", config.maxTries);

    printf(YEL "Byte error ratio: ");
    printf(CYN "%f%%.\n" RESET, config.byteErrorRatio);
}

void printMainMenuOptions()
{
    printf("What would you like to do?\n");

    printf("1 - Send file.\n");

    printf("2 - Receive file.\n");

    printf("3 - Set the port.\n");

    printf("4 - Set the baudrate.\n");

    printf("5 - Set the frame size.\n");

    printf("6 - Set the timeout.\n");

    printf("7 - Set the max number of retransmissions.\n");

    printf("8 - Set the byte error ratio.\n");

    printf("9 - Exit.\n");
}

void interfaceSendFile()
{
    printf("Which file would you like to transfer? ");
    char filename[MAX_INPUT];
    if (fgets(filename, MAX_INPUT, stdin) == NULL)
    {
        printf(RED "Invalid file name.\n" RESET);
        pressEnter();
        return;
    }
    filename[strlen(filename) - 1] = '\0';

    printf(RED "File name: %s\n" RESET, filename);
    pressEnter();

    initApplicationLayer(config.port, config.baudrate, config.frameSize, config.timeout, config.maxTries,
                         config.byteErrorRatio);
    int res = sendFile(filename); //"RCOM/resources/pinguim.gif"
    if (res == -1)
    {
        printf(RED "Error while trying to send file.\n" RESET);
        pressEnter();
        return;
    }
    printf(GRN "The file was successfully sent!\n" RESET);
    pressEnter();
    return;
}

void interfaceReceiveFile()
{
    initApplicationLayer(config.port, config.baudrate, config.frameSize, config.timeout, config.maxTries,
                         config.byteErrorRatio);
    int res = receiveFile();
    if (res == -1)
    {
        printf(RED "Error while trying to receive file.\n" RESET);
        pressEnter();
        return;
    }
    printf(GRN "The file was successfully received!\n" RESET);
    pressEnter();
    return;
}

void readPort()
{
    clearScreen();
    printf("Choose the communication port:\n");
    printf("1 - COM1 (ttyS0).\n");
    printf("2 - COM2 (ttyS1).\n");
    printf("3 - COM3 (ttyS2).\n");

    unsigned char decision = (unsigned char) getchar();
    cleanBuffer();

    switch (decision)
    {
        case '1':
            config.port = COM1;
            break;
        case '2':
            config.port = COM2;
            break;
        case '3':
            config.port = COM3;
            break;
        default:
            printf(RED "Invalid port.\n" RESET);
            pressEnter();
            return;
            break;
    }

    printf(GRN "The communication port was successfully updated!\n" RESET);
    pressEnter();
}

void readBaudrate()
{
    clearScreen();
    printf("Valid baudrate values: \n");
    printBaudrates();
    printf("Choose a valid baudrate: ");

    long int newUiBaudrate;
    if (readLongInt(&newUiBaudrate) == -1)
    {
        printf(RED "No integer was read from the console.\n" RESET);
        pressEnter();
        return;
    }
    if (newUiBaudrate <= 0)
    {
        printf(RED "The baudrate must be a positive value.\n" RESET);
        pressEnter();
        return;
    }

    tcflag_t newBaudrate = getBaudrate(newUiBaudrate);
    if (newBaudrate == (tcflag_t) -1)
    {
        printf(RED "Invalid baudrate value.\n" RESET);
        pressEnter();
        return;
    }

    config.uiBaudrate = newUiBaudrate;
    config.baudrate = newBaudrate;

    printf(GRN "The baudrate was successfully updated!\n" RESET);
    pressEnter();
}

void readFrameSize()
{
    clearScreen();
    printf("The frame size defines the size of the I frames used at the datalink layer.\n");
    printf("It must be more than 6 (in Bytes).\n");
    printf("Choose a valid frame size: ");

    long int newFrameSize;
    if (readLongInt(&newFrameSize) == -1)
    {
        printf(RED "No integer was read from the console.\n" RESET);
        pressEnter();
        return;
    }
    if (newFrameSize <= 6)
    {
        printf(RED "The frame size must be a positive value larger than 6.\n" RESET);
        pressEnter();
        return;
    }

    config.frameSize = (size_t) newFrameSize;

    printf(GRN "The frame size was successfully updated!\n" RESET);
    pressEnter();
}

void readTimeout()
{
    clearScreen();
    printf("The timeout defines the number of seconds that the transmitter waits before resending a frame, if no acknowledgement is received.\n");
    printf("Choose a valid timeout: ");

    long int newTimeout;
    if (readLongInt(&newTimeout) == -1)
    {
        printf(RED "No integer was read from the console.\n" RESET);
        pressEnter();
        return;
    }
    if (newTimeout <= 0)
    {
        printf(RED "The timeout must be a positive value.\n" RESET);
        pressEnter();
        return;
    }

    config.timeout = (unsigned int) newTimeout;

    printf(GRN "The timeout was successfully updated!\n" RESET);
    pressEnter();
}

void readMaxTries()
{
    clearScreen();
    printf("The max number of retransmissions defines the number of times that the transmitter sends the same frame without getting an acknowledgement before giving up.\n");
    printf("Choose a valid number: ");

    long int newMaxTries;
    if (readLongInt(&newMaxTries) == -1)
    {
        printf(RED "No integer was read from the console.\n" RESET);
        pressEnter();
        return;
    }
    if (newMaxTries <= 0)
    {
        printf(RED "The max number of retransmissions must be a positive value.\n" RESET);
        pressEnter();
        return;
    }

    config.maxTries = (unsigned int) newMaxTries;

    printf(GRN "The max number of retransmissions was successfully updated!\n" RESET);
    pressEnter();
}

void readByteErrorRatio()
{
    clearScreen();
    printf("The byte error ratio defines the proportion of bytes whose value has been artificially modified to the receiver end, to simulate errors at the datalink layer.\n");
    printf("Choose a valid ratio (in percentage): ");

    double newBer;
    if (readDouble(&newBer) == -1)
    {
        printf(RED "No double was read from the console.\n" RESET);
        pressEnter();
        return;
    }
    if (newBer < 0)
    {
        printf(RED "The byte error ratio must be a non-negative value.\n" RESET);
        pressEnter();
        return;
    }
    if (newBer > 100)
    {
        printf(RED "The byte error ratio must be less or equal to 100.\n" RESET);
        pressEnter();
        return;
    }

    config.byteErrorRatio = newBer;

    printf(GRN "The byte error ratio was successfully updated!\n" RESET);
    pressEnter();
}

int main()
{
    printf(RED "red\n" RESET);
    printf(GRN "green\n" RESET);
    printf(YEL "yellow\n" RESET);
    printf(BLU "blue\n" RESET);
    printf(MAG "magenta\n" RESET);
    printf(CYN "cyan\n" RESET);
    printf(WHT "white\n" RESET);

    initCommunicationConfig();
    unsigned char decision = 0;

    while (decision != '9')
    {
        clearScreen();
        printMainMenu();
        decision = (unsigned char) getchar();
        cleanBuffer();

        switch (decision)
        {
            case '1':
                interfaceSendFile();
                break;
            case '2':
                interfaceReceiveFile();
                break;
            case '3':
                readPort();
                break;
            case '4':
                readBaudrate();
                break;
            case '5':
                readFrameSize();
                break;
            case '6':
                readTimeout();
                break;
            case '7':
                readMaxTries();
                break;
            case '8':
                readByteErrorRatio();
                break;
            default:
                break;
        }
    }

    return 0;
}
