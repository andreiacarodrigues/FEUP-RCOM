#include <stdio.h> //printf

#include "shared.h"

void printByteByByte(char *str, unsigned int numBytes)
{
    if (numBytes == 0)
    {
        return;
    }

    printf("%x", str[0]);

    unsigned int i;
    for (i = 1; i < numBytes; i++)
    {
        printf(" %x", (unsigned char) str[i]);
    }
}