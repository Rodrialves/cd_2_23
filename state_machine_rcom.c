#include <stdio.h>
#include <stdlib.h>


/************** STATES ***************/
#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP 5


/************** FLAGS ***************/
#define FLAG 0x5c
#define A 0x01
#define C_SET 0x03
#define C_UA 0x07 




int main(){

    int i = 0, state = START;
    unsigned char aux;

    while(aux!='\n'){

        aux = getchar();
        if(aux=='\n')
            break;
        
        switch (state){

            case START:
                if(aux == FLAG){
                    state = FLAG_RCV;
                }
                break;
            
            case FLAG_RCV:
                if(aux == A){
                    state = A_RCV;
                }
                if(aux == FLAG_RCV){
                    break;
                }
                else{
                    state = START;
                }
                break;

            case A_RCV:
                if(aux == FLAG){
                    state = FLAG_RCV;
                }
                if(aux = C_SET){
                    state = C_RCV;
                }
                else{
                    state = START;
                }
                break;
            case C_RCV:
                unsigned char x = A^C_SET;
                if(aux == FLAG){
                    state = FLAG_RCV;
                }
                if(aux == x){
                    state = BCC_OK;
                }
                else{
                    state = START;
                }
                break;
            case BCC_OK:
                if(aux == FLAG){
                    state = STOP; /* Chegar aqui é bom */
                }
                else{
                    state = START;
                }
                break;
            }
        }

    return 0;
}