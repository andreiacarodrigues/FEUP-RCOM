#include <stdlib.h> //malloc
#include <stdio.h> //printf

#include "datalink.h"
#include "application.h"
#include "shared.h"

void testOpenNClose()
{
    initDatalinkLayer(9600, 256, 1, 10, 0.0);
    int fd = llopen(COM1, TRANSMIT);

    printf("llopen returned fd = %d\n", fd);
    printf("Sleeping for 3 secs.\n");

    int res = llclose(fd);

    printf("llclose returned with value = %d\n", res);
}

void testBuildSUFrame()
{
    initDatalinkLayer(9600, 256, 1, 10, 0.0);
    int fd = llopen(COM1, TRANSMIT);
    printf("llopen returned fd = %d\n", fd);

    //SET Frame
    char *frame = buildSUFrame(SET);
    printf("SET frame: ");
    printByteByByte(frame, SU_FRAME_SIZE);
    free(frame);

    //DISC Frame
    frame = buildSUFrame(DISC);
    printf("\nDISC frame: ");
    printByteByByte(frame, SU_FRAME_SIZE);
    free(frame);

    //UA Frame
    frame = buildSUFrame(UA);
    printf("\nUA frame: ");
    printByteByByte(frame, SU_FRAME_SIZE);
    free(frame);

    //RR Frame
    frame = buildSUFrame(RR);
    printf("\nRR frame: ");
    printByteByByte(frame, SU_FRAME_SIZE);
    free(frame);

    //REJ Frame
    frame = buildSUFrame(REJ);
    printf("\nREJ frame: ");
    printByteByByte(frame, SU_FRAME_SIZE);
    free(frame);

    //I Frame
    char strA[5] = "abcde";
    frame = buildIFrame(strA, 5);
    printf("\nI frame: ");
    printByteByByte(frame, I_FRAME_BASE_SIZE + 5);
    free(frame);

    int res = llclose(fd);

    printf("llclose returned with value = %d\n", res);
}

void testLLWriteOnce()
{
    initDatalinkLayer(9600, 256, 1, 10, 0.0);
    int fd = llopen(COM1, TRANSMIT);
    printf("llopen returned fd = %d\n", fd);

    char strA[] = "Ganda pena";
    int res = llwrite(fd, strA, sizeof(strA));
    printf("llwrite returned with value = %d\n", res);

    res = llclose(fd);

    printf("llclose returned with value = %d\n", res);
}

void testSendFile()
{
    initApplicationLayer(COM1, 9600, 256, 1, 10, 0.0);
    int res = sendFile("RCOM/resources/pinguim.gif");
    printf("sendFile returned with value = %d\n", res);
}

int main()
{
    /*//testOpenNClose
    printf("openNClose test \n");
    testOpenNClose();

    //testBuildSUFrame
    printf("BuildSUFrame test \n");
    testBuildSUFrame();

    //testLLWriteOnce
    printf("LLWriteOnce test \n");
    testLLWriteOnce();*/

    //testSendFile
    printf("sendFile test \n");
    testSendFile();

    printf("\nEnd of tests.\n");
    return 0;
}

