#include <stdlib.h> //malloc

#include "datalink.h"
#include "shared.h"

void stuffNDestuff(char **frame, unsigned int frameSize)
{
    printf("\nframe(%d bytes): ", frameSize);
    printByteByByte(*frame, frameSize);

    unsigned int newNBytes = stuff(frame, frameSize);
    printf("\nstuffed(%d bytes): ", newNBytes);
    printByteByByte(*frame, newNBytes);

    unsigned int oldNBytes = destuff(frame, newNBytes);
    printf("\ndestuffed(%d bytes): ", oldNBytes);
    printByteByByte(*frame, oldNBytes);
}

void testStuff()
{
    //Test 1, no stuffs
    unsigned int frameSize = 5;
    char *frame = (char *) malloc(sizeof(char) * frameSize);
    frame[0] = FLAG;
    frame[1] = (char) 0x12;
    frame[2] = (char) 0x34;
    frame[3] = (char) 0x56;
    frame[4] = FLAG;

    printf("test 1");
    stuffNDestuff(&frame, frameSize);

    //Test 2, FLAG stuff
    frameSize = 6;
    frame = (char *) realloc(frame, sizeof(char) * frameSize);
    frame[0] = FLAG;
    frame[1] = (char) 0x12;
    frame[2] = FLAG;
    frame[3] = (char) 0x34;
    frame[4] = (char) 0x56;
    frame[5] = FLAG;

    printf("\ntest 2");
    stuffNDestuff(&frame, frameSize);

    //Test 3, ESCAPE stuff
    frameSize = 6;
    frame = (char *) realloc(frame, sizeof(char) * frameSize);
    frame[0] = FLAG;
    frame[1] = (char) 0x12;
    frame[2] = ESCAPE;
    frame[3] = (char) 0x34;
    frame[4] = (char) 0x56;
    frame[5] = FLAG;

    printf("\ntest 3");
    stuffNDestuff(&frame, frameSize);

    //Test 4, FLAG and ESCAPE stuffs
    frameSize = 8;
    frame = (char *) realloc(frame, sizeof(char) * frameSize);
    frame[0] = FLAG;
    frame[1] = (char) 0x12;
    frame[2] = ESCAPE;
    frame[3] = FLAG;
    frame[4] = (char) 0x34;
    frame[5] = (char) 0x56;
    frame[6] = ESCAPE;
    frame[7] = FLAG;

    printf("\ntest 4");
    stuffNDestuff(&frame, frameSize);

    free(frame);
}

void testBaudrate()
{
    tcflag_t b300 = getBaudrate(300);
    printf("b300 = %d.", b300);

    tcflag_t b301 = getBaudrate(301);
    printf("\nb301 = %d.", b301);
}

int main()
{
    //(De)Stuff test
    printf("------------ (De)Stuff test ------------\n");
    testStuff();

    //Baudrate test
    printf("\n------------ Baudrate test ------------\n");
    testBaudrate();

    printf("\n------------ End of tests ------------\n");
    return 0;
}

