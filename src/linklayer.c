#include "linklayer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/************** GLOBAL VARIABLES ***************/
volatile int STOP = FALSE;
int tries = 0, fd;
struct termios oldtio, newtio;

/************** DEFAULTS ***************/
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS4"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define TIMEOUT 30
#define FALSE 0
#define TRUE 1

/************** STATES ***************/
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP_1 5

/************** VALUES ***************/
#define FLAG 0x5c
#define A_SET 0x01
#define A_UA 0x03
#define C_SET 0x03
#define C_UA 0x07

int write_func(unsigned char *vec)
{
    int i = 0, res, total = 0;
    unsigned char y;

    for (i = 0; i < 5; i++)
    {
        y = vec[i];
        res = write(fd, &y, 1);
        printf("%d bytes written\n", res);
        total += res;
    }

    if (total != 5)
        return -1;

    return 0;
}

int llopen(linkLayer connectionParameters)
{
    unsigned char aux, x, set[5], ua[5];
    int state = 0, res, i = 0;

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = connectionParameters.timeOut;
    newtio.c_cc[VMIN] = 0;

    if (connectionParameters.role == 1)
    {
        newtio.c_cc[VTIME] = connectionParameters.timeOut * 3;
        newtio.c_cc[VMIN] = 0;
    }

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    if (connectionParameters.role == 0)
    {
        STOP = FALSE;
        tries = 0;
        state = START;

        set[0] = FLAG;
        set[1] = A_SET;
        set[2] = C_SET;
        set[3] = set[1] ^ set[2];
        set[4] = FLAG;

        if (write_func(set) == -1)
            return -1;

        sleep(1);

        while (STOP == FALSE)
        {
            res = read(fd, aux, 1);

            if ((res <= 0) && (state == 0))
            {
                if (tries < connectionParameters.numTries)
                {
                    if (write_func(set) == -1)
                        return -1;
                }

                else
                {
                    printf("Can't connect to receiver\n");
                    return -1;
                }

                tries++;
            }

            switch (state)
            {

            case START:
                if (aux == FLAG)
                {
                    state = FLAG_RCV;
                }
                break;

            case FLAG_RCV:
                if (aux == A_UA)
                {
                    state = A_RCV;
                }
                else if (aux == FLAG_RCV)
                {
                    continue;
                }
                else
                {
                    state = START;
                }
                break;

            case A_RCV:
                if (aux == FLAG)
                {
                    state = FLAG_RCV;
                }
                if (aux = C_UA)
                {
                    state = C_RCV;
                }
                else
                {
                    state = START;
                }
                break;
            case C_RCV:

                x = A_UA ^ C_UA;
                if (aux == FLAG)
                {
                    state = FLAG_RCV;
                }
                if (aux == x)
                {
                    state = BCC_OK;
                }
                else
                {
                    state = START;
                }
                break;
            case BCC_OK:
                if (aux == FLAG)
                {
                    state = STOP_1;
                    STOP = TRUE;
                }
                else
                {
                    state = START;
                }
                break;
            }
        }

        return 1;
    }

    if (connectionParameters.role == 1)
    {
        while (STOP == FALSE)
        {                           /* loop for input */
            res = read(fd, aux, 1); /* returns after 1 chars have been input */

            if (res <= 0)
            {
                printf("Timeout on RCV");
                return -1;
            }

            switch (state)
            {

            case START:
                if (aux == FLAG)
                {
                    state = FLAG_RCV;
                }
                break;

            case FLAG_RCV:
                if (aux == A_SET)
                {
                    state = A_RCV;
                }
                else if (aux == FLAG_RCV)
                {
                    continue;
                }
                else
                {
                    state = START;
                }
                break;

            case A_RCV:
                if (aux == FLAG)
                {
                    state = FLAG_RCV;
                }
                if (aux = C_SET)
                {
                    state = C_RCV;
                }
                else
                {
                    state = START;
                }
                break;

            case C_RCV:

                x = A_SET ^ C_SET;
                if (aux == FLAG)
                {
                    state = FLAG_RCV;
                }
                if (aux == x)
                {
                    state = BCC_OK;
                }
                else
                {
                    state = START;
                }
                break;

            case BCC_OK:
                if (aux == FLAG)
                {
                    state = STOP_1;
                    STOP = TRUE;
                }
                else
                {
                    state = START;
                }
                break;
            }
        }

        sleep(2);

        ua[0] = FLAG;
        ua[1] = A_UA;
        ua[2] = C_UA;
        ua[3] = set[1] ^ set[2];
        ua[4] = set[0];

        if (write_func(ua))
            return -1;

        return 1;
    }

    return -1;
}

// Sends data in buf with size bufSize
int llwrite(char *buf, int bufSize)
{
    return 1;
}

// Receive data in packet
int llread(char *packet)
{
    return 1;
}

// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(linkLayer connectionParameters, int showStatistics)
{
    return 1;
}