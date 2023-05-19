/*Non-Canonical Input Processing*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

/************** FLAGS ***************/
#define FLAG 0x5c
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

volatile int STOP = FALSE;
int tries = 0;

/************** FUNCTIONS ***************/
void write_func(int fd, unsigned char *vec)
{
    int i = 0, res;
    unsigned char y;

    for (i = 0; i < 5; i++)
    {
        y = vec[i];
        res = write(fd, &y, 1);
        printf("%d bytes written\n", res);
    }
}

int main(int argc, char **argv)
{
    int fd, c, res;
    struct termios oldtio, newtio;
    unsigned char buf[255], set[5], aux1, aux[2], printer[255];
    int i, sum = 0, speed = 0, state = 0;
    unsigned char x, y;

    if ((argc < 2) ||
        ((strcmp("/dev/ttyS1", argv[1]) != 0) &&
         (strcmp("/dev/ttyS4", argv[1]) != 0)))
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

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = TIMEOUT; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;        /* blocking read until 1 char(s) received */

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

    for (i = 0; i < 255; i++)
    {

        buf[i] = '\0';
    }

    i = 0;

    set[0] = FLAG;
    set[1] = 0x01;
    set[2] = 0x03;
    set[3] = set[1] ^ set[2];
    set[4] = FLAG;

    write_func(fd, set);

    sleep(1);

    while (STOP == FALSE)
    {                           /* loop for input */
        res = read(fd, aux, 1); /* returns after 1 chars have been input */

        printf("%i\n\n", res);
        if ((res <= 0) && (state == 0))
        {
            if (tries <= 2)
                write_func(fd, set);
            else
            {
                printf("Can't connect to receiver\n");
                exit(-1);
            }
            tries++;
            // printf("%d",tries);
        }

        aux[res] = '\0'; /* so we can printf... */
        printer[i] = aux[0];
        i++;
        aux1 = aux[0];
        // printf(":%x:\n",aux1);

        switch (state)
        {

        case START:
            if (aux1 == FLAG)
            {
                state = FLAG_RCV;
            }
            break;

            f("%d\n", state);
        case FLAG_RCV:
            if (aux1 == A)
            {
                state = A_RCV;
            }
            else if (aux1 == FLAG_RCV)
            {
                continue;
            }
            else
            {
                state = START;
            }
            break;

        case A_RCV:
            if (aux1 == FLAG)
            {
                state = FLAG_RCV;
            }
            if (aux1 = C_UA)
            {
                state = C_RCV;
            }
            else
            {
                state = START;
            }
            break;
        case C_RCV:

            x = A ^ C_UA;
            if (aux1 == FLAG)
            {
                state = FLAG_RCV;
            }
            if (aux1 == x)
            {
                state = BCC_OK;
            }
            else
            {
                state = START;
            }
            break;
        case BCC_OK:
            if (aux1 == FLAG)
            {
                state = STOP_1; /* Chegar aqui é bom */
                STOP = TRUE;
            }
            else
            {
                state = START;
            }
            break;
        }
        printf("\n\n%i", STOP);
        // printf("%d\n",state);
    }

    // printf("\n%s\n",printer);

    for (i = 0; i < 5; i++)
    {
        // printf("\n%x\n",printer[i]);
    }

    /*

    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar

    o indicado no guião

    */

    sleep(1);

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
