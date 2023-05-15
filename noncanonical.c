/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
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
#define A 0x01
#define C_SET 0x03
#define C_UA 0x07 

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res, i=0, state=START;
    struct termios oldtio,newtio;
    unsigned char buf[255], printer[255], aux[2], x, aux1, set[5], z;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    
    for(int j=0;j<255;j++){
    printer[j]='\0';
    }

    printf("New termios structure set\n");

    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,aux,1);   /* returns after 1 chars have been input */         
        aux[res]='\0'; /* so we can printf... */
        printf(":%x:%d\n", aux[0], res);
        printer[i] = aux[0];
        if (aux[0]=='\0'){STOP=1;}
        i++;
        aux1=aux[0];
        
        switch (state){

            case START:
                if(aux1 == FLAG){
                     state = FLAG_RCV;
                }
                break;
            
            case FLAG_RCV:
                if(aux1 == A){
                    state = A_RCV;
                }
                else if(aux1 == FLAG_RCV){
                    continue;                    
                }
                else{
                    state = START;
                }
                break;

            case A_RCV:
                if(aux1 == FLAG){
                    state = FLAG_RCV;
                }
                if(aux1 = C_SET){
                    state = C_RCV;
                }
                else{
                    state = START;
                }
                break;
            case C_RCV:

                x = A^C_SET;
                if(aux1 == FLAG){
                    state = FLAG_RCV;
                }
                if(aux1 == x){
                    state = BCC_OK;
                }
                else{
                    state = START;
                }
                break;
            case BCC_OK:
                if(aux1 == FLAG){
                    state = STOP_1; /* Chegar aqui é bom */
                    STOP=TRUE;
                }
                else{
                    state = START;
                }
                break;
            }
            printf("%d\n",state);
        }
    
        
        // printf("\n%s\n",printer);

        for (int i=0;i<5;i++){
            printf("\n%x\n",printer[i]);
        }

    sleep(2);

    set[0]=0x5c;
    set[1]=0x03;
    //set[2]=0x07;             /* Não esquecer tirar */
    set[2]=0x09;
    set[3]=set[1]^set[2];
    set[4]=set[0];
        
    for(int n=0;n<5;n++){
        z=set[n];
        res = write(fd, &z, 1);
                    
    }

    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
