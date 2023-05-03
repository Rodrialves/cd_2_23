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

#define FALSE 0 

#define TRUE 1 

 
 

volatile int STOP=FALSE; 

 
 

int main(int argc, char** argv) 

{ 

int fd,c, res; 

struct termios oldtio,newtio; 

unsigned char buf[255], set[5]; 

int i, sum = 0, speed = 0; 

unsigned char x; 

 
 

if ( (argc < 2) || 

((strcmp("/dev/ttyS0", argv[1])!=0) && 

(strcmp("/dev/ttyS4", argv[1])!=0) )) { 

printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n"); 

exit(1); 

} 

 
 
 

/* 

Open serial port device for reading and writing and not as controlling tty 

because we don't want to get killed if linenoise sends CTRL-C. 

*/ 

 
 
 

fd = open(argv[1], O_RDWR | O_NOCTTY ); 

if (fd < 0) { perror(argv[1]); exit(-1); } 

 
 

if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */ 

perror("tcgetattr"); 

exit(-1); 

} 

 
 

bzero(&newtio, sizeof(newtio)); 

newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD; 

newtio.c_iflag = IGNPAR; 

newtio.c_oflag = 0; 

 
 

/* set input mode (non-canonical, no echo,...) */ 

newtio.c_lflag = 0; 

 
 

newtio.c_cc[VTIME] = 0; /* inter-character timer unused */ 

newtio.c_cc[VMIN] = 5; /* blocking read until 5 chars received */ 

 
 
 
 

/* 

VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 

leitura do(s) próximo(s) caracter(es) 

*/ 

 
 
 

tcflush(fd, TCIOFLUSH); 

 
 

if (tcsetattr(fd,TCSANOW,&newtio) == -1) { 

perror("tcsetattr"); 

exit(-1); 

} 

 
 

printf("New termios structure set\n"); 

 
 
 
 

for (i = 0; i < 255; i++) { 

buf[i] = '\0'; 

} 

i=0; 

 
 

/*while(x!='\n'){ 

x = getchar(); 

buf[i]=x; 

if(x=='\n')

buf[i]='\0'; 

i++; 

}*/ 

    /*scanf("%s",&buf); 

    for(i=0; i<255;i++){ 

    /*if(strcmp(buf[i],"/n")==0){ 

    buf[i]="/0"; 

    break; 

    } 

    }*/ 

    set[2]=0x03;
    set[0]=0x5c;
    set[1]=0x01;
    set[3]=set[1]^set[2];
    set[4]=set[0];

    /*for (int i=0;i<5;i++){
            printf("\n%x\n",set[i]);
        }*/
    

    res = write(fd,set,5); 

    printf("%d bytes written\n", res); 

 
 

    /* 

    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 

    o indicado no guião 

    */ 
 

    sleep(1); 

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) { 
        perror("tcsetattr"); 
        exit(-1); 
    } 

    close(fd); 
    return 0; 
} 
