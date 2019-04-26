#include "main.h"
#include <time.h>
MPI_Datatype MPI_PAKIET_T;
pthread_t threadCom, threadM;

/* zamek do synchronizacji zmiennych współdzielonych */
pthread_mutex_t zegar_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stan_mut = PTHREAD_MUTEX_INITIALIZER;
sem_t all_sem;

int zegarLamporta=0;
int stan=CHCE_BRON;
int idOstatiegoRequesta=0;
int otrzymaneZgody=0;
int chcianaBron=0;

/* end == TRUE oznacza wyjście z main_loop */
volatile char end = FALSE;
void mainLoop(void);

/* Deklaracje zapowiadające handlerów. */
void myStateHandler(packet_t *pakiet);
void finishHandler(packet_t *pakiet);
void appMsgHandler(packet_t *pakiet);
void giveHandler(packet_t *pakiet);

/* typ wskaźnik na funkcję zwracającej void i z argumentem packet_t* */
typedef void (*f_w)(packet_t *);
/* Lista handlerów dla otrzymanych pakietów
   Nowe typy wiadomości dodaj w main.h, a potem tutaj dodaj wskaźnik do 
     handlera.
   Funkcje handleróœ są na końcu pliku. Nie zapomnij dodać
     deklaracji zapowiadającej funkcji!
*/

#define JO_CHCA 0
#define POZWALAM 1
#define UMIERAM 2
#define PLACEHOLDER 3 // zmienic
#define ZAJMUJE_BANDERSNACHOWNIE 4
// #define CHCE_MIEJSCE_W_WYPYCHALNI 5 - juz jest

f_w handlers[MAX_HANDLERS] = { [JO_CHCA]=giveHandler,
            [FINISH] = finishHandler,
            [MY_STATE_IS] = myStateHandler,
            [APP_MSG] = appMsgHandler 
            []};

extern void inicjuj(int *argc, char ***argv);
extern void finalizuj(void);

int getZegar(int inkrementowac){
    int a=-1;
    pthread_mutex_lock(&zegar_mut);
    zegarLamporta+=inkrementowac;
    a=zegarLamporta;
	pthread_mutex_unlock(&zegar_mut);
    return a;
}

int getStan(){
     int a;
    pthread_mutex_lock(&stan_mut);
    a=stan;
	pthread_mutex_unlock(&stan_mut);
    return a;
}

// void getIdOStatniegoRequesta(int inkrementowac){
//     int a=-1;
//     pthread_mutex_lock(&zegar_mut);
//     zegarLamporta+=inkrementowac;
//     a=zegarLamporta;
// 	pthread_mutex_unlock(&zegar_mut);
//     return a;
// }

void zmienStan(int naCo){
    pthread_mutex_lock(&stan_mut);
    stan=naCo;
	pthread_mutex_unlock(&stan_mut);
}

int setZegarToMax(int new){
    int a=-1;
    pthread_mutex_lock(&zegar_mut);
    if(new>zegarLamporta) zegarLamporta=new;
    zegarLamporta++;
    a=zegarLamporta;
	pthread_mutex_unlock(&zegar_mut);
    return a;

}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc,&argv);

    mainLoop();

    finalizuj();
    return 0;
}


/* Wątek główny - przesyłający innym pieniądze */
void mainLoop(void)
{
    srand(time(NULL));
    int dst;
    packet_t pakiet;
    /* mały sleep, by procesy nie zaczynały w dokładnie tym samym czasie */
    struct timespec t = { 0, rank*50000 };
    struct timespec rem = { 1, 0 };
    nanosleep(&t,&rem); 
    //

    /* pętla główna: sen, wysyłanie przelewów innym bankom */
    while (!end) {
	    int percent = rand()%2 + 1;
        struct timespec t = { percent, 0 };
        struct timespec rem = { 1, 0 };
        nanosleep(&t,&rem);
        switch (stan){
            case CHCE_BRON:
            if((float)rand()/RAND_MAX>0.5)chcianaBron=MIECZ;
            ifnt chcianaBron=KARABIN;
            
            println(" Jo chca %d\n",chcianaBron);
            
            pthread_mutex_lock(&zgoda_mut);

            for(int i=0;i<size;i++){
                if(rank!=i){
                    packet_t tmp;
                    tmp.tresc = chcianaBron; 
                    tmp.ts=getZegar(1);
                    tmp.id=idOstatiegoRequesta++;
                    
                    sendPacket(&tmp, i, JO_CHCA);
                }
            }
            pthread_mutex_lock(&zgoda_mut);
            println(" Dostalem zgody,zaczynam polowac\n");
            zmienStan(POLUJE);
            // sendPacket(&pakiet, dst, APP_MSG);
            break;
            
        }  
    }
}

/* Wątek komunikacyjny - dla każdej otrzymanej wiadomości wywołuje jej handler */
void *comFunc(void *ptr)
{

    MPI_Status status;
    packet_t pakiet;
    /* odbieranie wiadomości */
    while ( !end ) {
	// println("[%d] czeka na recv\n", rank);
        MPI_Recv( &pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pakiet.src = status.MPI_SOURCE;

        handlers[(int)status.MPI_TAG](&pakiet); // zamiast wielkiego switch status.MPI_TAG... case X: handler()
    }
    println(" Koniec! ");
    return 0;
}

/* Handlery */
void myStateHandler(packet_t *pakiet)
{
    setZegarToMax(pakiet->ts);
    
    
   
    //println( "%d statePackets from %d\n", statePacketsCnt, pakiet->src);
    
    
}

void finishHandler( packet_t *pakiet)
{
    setZegarToMax(pakiet->ts);
    println("Otrzymałem FINISH" );
    end = TRUE; 
}

void giveHandler( packet_t *pakiet)
{
    /* monitor prosi, by mu podać stan kasy */
    /* tutaj odpowiadamy monitorowi, ile mamy kasy. Pamiętać o muteksach! */
    setZegarToMax(pakiet->ts);
    println("dostałem GIVE STATE");
   
    packet_t tmp;
    tmp.ts=getZegar(1);
    // sendPacket(&tmp, ROOT, MY_STATE_IS);
}

void appMsgHandler( packet_t *pakiet)
{
    /* ktoś przysłał mi przelew */
    
    setZegarToMax(pakiet->ts);
}
