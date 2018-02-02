/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datalink.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define BUF_SIZE 255

int main(int argc, char **argv)
{
    int fd; //file descriptor for the serial port
    struct termios newtio; //termios struct to edit the read/write properties in fd
    struct termios oldtio; //original termios struct
    char buf[BUF_SIZE]; //array to store 

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
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio)); //fill newtio with 0's
    // CS8: 8n1 (8 bits, sem bit de paridade,1 stopbit)
    // CLOCAL: ligação local, sem modem
    // CREAD: activa a recepção de caracteres
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    // IGNPAR: Ignora erros de paridade
    newtio.c_iflag = IGNPAR;
    //Saida nao processada
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN] = 1;   /* blocking read until 5 chars received */


    /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    //Read text from keyboard
    int enviados = 0;
    printf("Please write the message to send: ");
    fgets(buf, BUF_SIZE - 1, stdin);
    int total = strlen(buf) + 1;    //number of bytes to be transmitted (includes both a '\n' and a '\0')
    printf("Msg to be sent: %s", buf);
    printf("Num bytes to be sent: %d\n", total);
    printf("strlen(buf) = %lu\n", strlen(buf));

    //Send the text
    while (enviados < total)
    {
        int n; //number of bytes written
        if ((n = write(fd, buf + enviados, total - enviados)) == -1)
        {
            perror("write");
            exit(-1);
        }
        enviados += n;
    }


    //Read the message sent back
    char message[BUF_SIZE]; //array to store the whole message

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


    //Reset terminal control config
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);

    printf("Success!\n");
    return 0;
}
