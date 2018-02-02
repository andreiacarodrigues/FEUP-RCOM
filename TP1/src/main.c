#include <stdio.h> //perror
#include <unistd.h> //read
#include <stdlib.h> //malloc
#include "applicationLayer.h"
#include "interface.h"

#define DEFAULT_BAUDRATE 9600
#define DEFAULT_MAX_DATA_SIZE 512
#define DEFAULT_MAX_NUM_TIMEOUTS 3
#define DEFAULT_TIMEOUT 3

void testBuildDPackage()
{
    char *myPackage = "Estou a mandar um pacote";

    char *myDPackage = buildDPackage(sizeof(char) * 24, myPackage);

    printf("Data Package built: \n");

    unsigned int i = 0;
    for (i; i < (sizeof(char) * (24 + 4)); i++)
    {
        printf("0x%x ", myDPackage[i]);
    }

    printf("\n\n");
}

void testBuildCPackage()
{
    printf("Start Control Package: \n");

    char *myStartControlPackage = buildCPackage(START, "00001456", "MyFileName");

    unsigned int i = 0;
    for (i; i < (sizeof(char) * (10 + 8 + 5)); i++)
    {
        printf("0x%x ", myStartControlPackage[i]);
    }

    printf("\n\n");

    // ----------------------------------------------------------------------------

    printf("End Control Package: \n");

    char *myEndControlPackage = buildCPackage(END, "00001456", "MyFileName");

    i = 0;
    for (i; i < (sizeof(char) * (10 + 8 + 5)); i++)
    {
        printf("0x%x ", myEndControlPackage[i]);
    }

    printf("\n\n");
}

int main(int argc, char **argv)
{

    if (argc != 5)
    {
        printf("Error! Usage: %s <file name to send/receive> <baud rate> <max data size of frame I> <max num timeouts> <timeout>. For default values, use -1 in the respective argument.",
               argv[0]);
        return 1;
    }

    char *fileName = argv[1];
    unsigned int baudrate;
    unsigned int maxDataSize;
    unsigned int maxNumTimeouts;
    unsigned int timeout;

    if (argv[2] > 0)
    {
        baudrate = strtol(argv[2], NULL, 10);
    }
    else
    {
        baudrate = DEFAULT_BAUDRATE;
    }

    if (argv[3] > 0)
    {
        maxDataSize = strtol(argv[3], NULL, 10);
    }
    else
    {
        maxDataSize = DEFAULT_MAX_DATA_SIZE;
    }

    if (argv[4] > 0)
    {
        maxNumTimeouts = strtol(argv[4], NULL, 10);
    }
    else
    {
        maxNumTimeouts = DEFAULT_MAX_NUM_TIMEOUTS;
    }

    if (argv[5] > 0)
    {
        timeout = strtol(argv[5], NULL, 10);
    }
    else
    {
        timeout = DEFAULT_TIMEOUT;
    }


    int connectionMode = startInterface();

    initApplicationLayer(connectionMode, fileName, baudrate, maxDataSize, maxNumTimeouts, timeout);





    //testBuildDPackage();

    //testBuildCPackage();

    return 0;
}

