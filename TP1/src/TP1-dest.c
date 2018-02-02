/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h> //exit
#include <string.h> //strcmp
#include <unistd.h> //read

#include "datalink.h"

#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define BUFSIZE 255

int main(int argc, char **argv)
{
    int fd; //file descriptor
    struct termios newtio; //termios struct to edit the read/write properties in fd
    struct termios oldtio; //original termios struct

    //Check the arguments (1st arg must be /dev/ttySX, X = 0 or 1)
    if ((argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
         (strcmp("/dev/ttyS1", argv[1]) != 0)))
    {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
      Open serial port device for reading and writing and not as controlling tty
      because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(argv[1]);
        exit(-1);
    }

    //Terminal control
    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        close(fd);
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio)); //fill newtio with 0's)
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;   /* inter-character timer unused (tenth of a second) */
    newtio.c_cc[VMIN] = 1;   /* blocking read until 5 chars received */



    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) próximo(s) caracter(es)
    */



    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        close(fd);
        exit(-1);
    }

    printf("New termios structure set\n");

    char message[BUFSIZE]; //array to store the whole message

    //Read text from serial port
    int res = readFromFd(fd, message);

    if (res == -1)
    {
        tcsetattr(fd, TCSANOW, &oldtio);
        close(fd);
        exit(-1);
    }

    printf("Message read: %s", message);
    printf("Num bytes read: %d\n", res);
    printf("strlen(message) = %lu\n", strlen(message));


    //Send the message back
    int enviados = 0;
    while (enviados < res)
    {
        int n; //number of bytes written
        if ((n = write(fd, message + enviados, res - enviados)) == -1)
        {
            perror("write");
            exit(-1);
        }
        enviados += n;
    }





    //Reset the terminal control config 
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        close(fd);
        exit(-1);
    }
    close(fd);
    return 0;
}
