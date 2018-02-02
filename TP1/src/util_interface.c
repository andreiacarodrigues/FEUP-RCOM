#include <stdio.h> //printf, getchar

#include "util_interface.h"

void cleanBuffer()
{
    // Clear buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {}
}

void clearScreen()
{
    printf("\033[H\033[J");
}