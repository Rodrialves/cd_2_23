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
int tries = 0, fd, state = 0, res;
struct termios oldtio, newtio;
unsigned char ua[5], aux, x;

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
#define DATA 6
#define STOP_2 7

/************** VALUES ***************/
#define FLAG 0x5c
#define A_SET 0x01
#define A_UA 0x03
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0b
#define C_I0 0x00
#define C_I1 0x02
#define ESC 0x5d 

/*
Verificar todos os valores!!!
*/

/************** CONTROL ***************/
#define SET 0
#define UA 1
#define DISC 2

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

void state_machine_control(int control)
{
    int a, c;

    if (control == SET)
    {
        a = A_SET;
        c = C_SET;
    }

    else if (control == UA)
    {
        a = A_UA;
        c = C_UA;
    }

    else if (control == DISC)
    {
        a = A_SET;
        c = C_DISC;
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

void state_machine_data()
{
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

        state = DATA;
        break;

    case DATA:
        if (aux == FLAG)
        {
            state = STOP_2;
            STOP = TRUE;
        }
        else
        {
            state = DATA;
        }
        break;
    }
}

int stuffing(unsigned char *vec, unsigned char *stuff, int len)
{
    int i = 0, j = 0;

    for (i = 0; i < len; i++)
    {

        stuff[j] = vec[i];

        if (vec[i] == FLAG)
        {
            stuff[j] = ESC;
            j++;
            stuff[j] = vec[i] ^ 0x20;
        }

        if (vec[i] == ESC)
        {
            stuff[j] = ESC;
            j++;
            stuff[j] = vec[i] ^ 0x20;
        }
        j++;
    }

    return j;
}

int destuffing(unsigned char *vec, unsigned char *unstuff, int len)
{
    int i = 0, j = 0, size;
    unsigned char bcc2;

    for (i = 0; i < len; i++)
    {

        unstuff[j] = vec[i];

        if ((vec[i] == ESC) && (vec[i + 1] == (FLAG ^ 0x20)))
        {
            unstuff[j] = FLAG;
            i++;
        }

        if ((vec[i] == ESC) && (vec[i + 1] == (ESC ^ 0x20)))
        {
            unstuff[j] = ESC;
            i++;
        }
        j++;
    }

    bcc2=unstuff[0]^unstuff[1];
    for(i=2;i<j-2;i++)
    {
        bcc2=bcc2^unstuff[i];
    }

    if(bcc2==unstuff[j-1]) return 1;

    return -1;
}

int llopen(linkLayer connectionParameters)
{
    unsigned char set[5];
    int i = 0;

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
            res = read(fd, aux, 1); /* Read UA message sent from rx */

            if ((res <= 0) && (state == 0)) /* If the message was sent but didn`t receive confirmation, tries again */
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

            state_machine_control(UA); /* Verifies that the UA message was received correctly*/

            printf("UA success\n");
        }

        return 1;
    }

    if (connectionParameters.role == 1)
    {
        while (STOP == FALSE)
        {
            res = read(fd, aux, 1); /* Read SET message sent from tx */

            if (res <= 0) /* When number of tries exceed the intended, ends read process */
            {
                printf("Timeout on RCV");
                return -1;
            }

            state_machine_control(SET); /* Verifies that the SET message was received correctly*/
        }

        printf("SET success\n");

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
    unsigned char stuff[bufSize * 2+6], bcc2, buf1[bufSize+6];
    int i = 0, stuff_len;

    bcc2=buf[0]^buf[1];

    for (i=2;i<bufSize;i++)
    {
        bcc2=bcc2^buf[i];
    }

    buf1[0]=FLAG;
    buf1[1]=A_SET;
    buf1[2]=C_I0;
    buf1[3]=buf1[1]^buf1[2];
    buf1[bufSize+4]=bcc2;
    buf1[bufSize+5]=FLAG;

    stuff_len = stuffing(buf1, stuff, bufSize);

    for (i; i < bufSize; i++)
    {
        res=write();
    }
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
    unsigned char disc[5];

    if (connectionParameters.role == 0)
    {
        STOP = FALSE;
        tries = 0;
        state = 0;
        int res;

        disc[0] = FLAG;
        disc[1] = A_SET;
        disc[2] = C_DISC;
        disc[3] = disc[1] ^ disc[2];
        disc[4] = disc[0];

        if (write_func(disc) == -1)
            return -1;

        while (STOP == FALSE)
        {
            res = read(fd, aux, 1);

            if ((res <= 0) && (state == 0))
            {
                if (tries < connectionParameters.numTries)
                {
                    if (write_func(disc) == -1)
                        return -1;
                }

                else
                {
                    printf("Can't connect to receiver\n");
                    return -1;
                }

                tries++;
            }

            state_machine_control(DISC);
        }

        printf("DISC success\n");

        if (write_func(ua) == -1)
            return -1;

        return 1;
    }

    if (connectionParameters.role == 1)
    {
        STOP = 0;
        state = START;

        while (STOP == FALSE)
        {                           /* loop for input */
            res = read(fd, aux, 1); /* returns after 1 chars have been input */

            if (res <= 0)
            {
                printf("Timeout on RCV");
                return -1;
            }

            state_machine_control(DISC);
        }

        printf("DISC success\n");

        STOP = FALSE;
        state = 0;

        if (write_func(disc) == -1)
            return -1;

        while (STOP == FALSE)
        {                           /* loop for input */
            res = read(fd, aux, 1); /* returns after 1 chars have been input */

            if (res <= 0)
            {
                printf("Timeout on RCV");
                return -1;
            }

            state_machine_control(UA);
        }

        printf("UA success\n");

        return 1;
    }

    return -1;
}