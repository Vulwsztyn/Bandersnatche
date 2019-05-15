#include "main.h"
#include <time.h>
MPI_Datatype MPI_PAKIET_T;
pthread_t threadCom, threadM;

/* zamek do synchronizacji zmiennych współdzielonych */
pthread_mutex_t zegar_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t currentId_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stan_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t potencjalne_miejsce_w_wypychalni_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t miejsce_niezajete_w_wypychalni_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t martwi_mut = PTHREAD_MUTEX_INITIALIZER;

//muteksy zgadzania sie
pthread_mutex_t miecz_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t karabin_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sanitariusz_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wypychalnia_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;

sem_t all_sem;

int zegarLamporta=0;
int stan=CHCE_BRON;

int idOstatiegoRequesta=0;
int otrzymaneZgody=0;

int juzWybralemBron=0;
int wybranaBron=0;

int rozmiarMojegoBandersnatcha=0;

int potencjalnieZajeteMiejsce=0;

int liczbaMartwychLowcow=0;

int miejsceNieZajetePrzezBedacychwWypychalni=0;
/* end == TRUE oznacza wyjście z main_loop */
volatile char end = FALSE;
void mainLoop(void);

/* Deklaracje zapowiadające handlerów. */
void joChcaHandler(packet_t *pakiet);
void pozwolenieHandler(packet_t *pakiet);
void zgonHandler(packet_t *pakiet);
void placeholderHandler(packet_t *pakiet);
void bandersnatchowniaHandler(packet_t *pakiet);

/* typ wskaźnik na funkcję zwracającej void i z argumentem packet_t* */
typedef void (*f_w)(packet_t *);
/* Lista handlerów dla otrzymanych pakietów
   Nowe typy wiadomości dodaj w main.h, a potem tutaj dodaj wskaźnik do 
     handlera.
   Funkcje handleróœ są na końcu pliku. Nie zapomnij dodać
     deklaracji zapowiadającej funkcji!
*/

f_w handlers[MAX_HANDLERS] = { [JO_CHCA]=joChcaHandler,
            [POZWALAM] = pozwolenieHandler,
            [UMIERAM] = zgonHandler,
            [PLACEHOLDER] = placeholderHandler,
            [BANDERSNACHOWNIA]=bandersnatchowniaHandler
            };

extern void inicjuj(int *argc, char ***argv);
extern void finalizuj(void);

pthread_mutex_t* bron_zgoda_mut(int co){
    if(co==MIECZ) return &miecz_zgoda_mut;
    if(co==KARABIN) return &karabin_zgoda_mut;
}

//TODO - przetestowac
int ustawRzeczzMuteksami(int *co,pthread_mutex_t *muteks,int naIle){
    int a=-1;
    pthread_mutex_lock(muteks);
    *co=naIle;
    a=*co;
	pthread_mutex_unlock(muteks);
    return a;
}


//TODO - przetestowac
int zwiekszRzeczzMuteksami(int *co,pthread_mutex_t *muteks,int oIle){
    int a=-1;
    pthread_mutex_lock(muteks);
    *co+=oIle;
    a=*co;
	pthread_mutex_unlock(muteks);
 
    return a;
}

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

void zerujZgody(){
    pthread_mutex_lock(&zgoda_mut);
    otrzymaneZgody=0;
    pthread_mutex_unlock(&zgoda_mut);
}

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

float myRandomFloat(){
    return (float)((float)rand()/ (float)RAND_MAX);    
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
                wybranaBron=(int)(myRandomFloat()*LICZBA_BRONI);
                juzWybralemBron=1;
                
                println(" Jo chca %d\n",wybranaBron);
                
                pthread_mutex_lock(&zgoda_mut);

                for(int i=0;i<size;i++){
                    if(rank!=i){
                        packet_t tmp;
                        tmp.tresc = wybranaBron; 
                        tmp.ts=getZegar(1);
                        idOstatiegoRequesta++;
                        tmp.id=idOstatiegoRequesta;
                        sendPacket(&tmp, i, JO_CHCA);
                    }
                }
                pthread_mutex_lock(&zgoda_mut);
                pthread_mutex_unlock(&zgoda_mut);

                println(" Dostalem zgody,zaczynam polowac\n");
                pthread_mutex_lock(bron_zgoda_mut(wybranaBron));
                zmienStan(POLUJE);
                juzWybralemBron=0;
                // sendPacket(&pakiet, dst, APP_MSG);
                break;
            case POLUJE:
                sleep(CZAS_POLOWANIA);
                pthread_mutex_unlock(bron_zgoda_mut(wybranaBron));
                float r=myRandomFloat()*100;
                if(r<SZANSA_ZGONU){
                    zmienStan(MARTWY);
                }
                ifnt{
                    r-=SZANSA_ZGONU;
                    if(r<SZANSA_RANNOSCI){
                        zmienStan(RANNY);
                    }
                    ifnt{
                        zmienStan(CHCE_MIEJSCE_W_WYPYCHALNI);
                    }
                }
                break;
            case MARTWY:
                end=TRUE;
                break;
            case RANNY:
                pthread_mutex_lock(&zgoda_mut);
                for(int i=0;i<size;i++){
                    if(rank!=i){
                        packet_t tmp;
                        tmp.tresc = MEDYK; 
                        tmp.ts=getZegar(1);
                        idOstatiegoRequesta++;
                        tmp.id=idOstatiegoRequesta;
                        sendPacket(&tmp, i, JO_CHCA);
                    }
                }
                pthread_mutex_lock(&zgoda_mut);
                pthread_mutex_unlock(&zgoda_mut);

                println(" Dostalem zgody,zaczynam sie leczyc\n");
                pthread_mutex_lock(&sanitariusz_zgoda_mut);
                zmienStan(LECZONY);
                break;
            case CHCE_MIEJSCE_W_WYPYCHALNI:
                pthread_mutex_lock(&zgoda_mut);
                rozmiarMojegoBandersnatcha=(int) myRandomFloat()*MAX_ROZMIAR_BANDERSNATCHA+1;
                //to jest zle
                //ustawRzeczzMuteksami(miejsceNieZajetePrzezBedacychwWypychalni,miejsce_niezajete_w_wypychalni_mut,MAX_ROZMIAR_BANDERSNATCHA*(size-zwiekszRzeczzMuteksami(liczbaMartwychLowcow,martwi_mut,0)));
                ustawRzeczzMuteksami(&potencjalnieZajeteMiejsce,&potencjalne_miejsce_w_wypychalni_mut,MAX_ROZMIAR_BANDERSNATCHA*(size-zwiekszRzeczzMuteksami(&liczbaMartwychLowcow,&martwi_mut,0)));
                
                for(int i=0;i<size;i++){
                    if(rank!=i){
                        packet_t tmp;
                        tmp.tresc = CHCE;
                        tmp.reszta = rozmiarMojegoBandersnatcha;
                        tmp.ts=getZegar(1);
                        idOstatiegoRequesta++;
                        tmp.id=idOstatiegoRequesta;
                        sendPacket(&tmp, i, BANDERSNACHOWNIA);
                    }
                }
                pthread_mutex_lock(&zgoda_mut);
                pthread_mutex_unlock(&zgoda_mut);
                for(int i=0;i<size;i++){
                    if(rank!=i){
                        packet_t tmp;
                        tmp.tresc = ZAJMUJE;
                        tmp.reszta = rozmiarMojegoBandersnatcha;
                        tmp.ts=getZegar(1);
                        idOstatiegoRequesta++;
                        tmp.id=idOstatiegoRequesta;
                        sendPacket(&tmp, i, BANDERSNACHOWNIA);
                    }
                }
                pthread_mutex_lock(&wypychalnia_zgoda_mut);
                zmienStan(SIEDZE_W_WYPYCHALNI);
                break;      
            case SIEDZE_W_WYPYCHALNI:
                sleep(CZAS_WYPYCHANIA);
                pthread_mutex_unlock(&wypychalnia_zgoda_mut);
                for(int i=0;i<size;i++){
                    if(rank!=i){
                        packet_t tmp;
                        tmp.tresc = ODDAJE;
                        tmp.reszta = rozmiarMojegoBandersnatcha;
                        tmp.ts=getZegar(1);
                        idOstatiegoRequesta++;
                        tmp.id=idOstatiegoRequesta;
                        sendPacket(&tmp, i, BANDERSNACHOWNIA);
                    }
                }
                zmienStan(JO_CHCA);
                break;      
            case LECZONY:
                sleep(CZAS_LECZENIA);
                pthread_mutex_unlock(&sanitariusz_zgoda_mut);
                zmienStan(JO_CHCA);
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
        //TODO - test this shit
        if(!fork()){
            handlers[(int)status.MPI_TAG](&pakiet); // zamiast wielkiego switch status.MPI_TAG... case X: handler()
        }
    }
    println(" Koniec! ");
    return 0;
}

/* Handlery */
void joChcaHandler(packet_t *pakiet)
{
    switch (pakiet->tresc){
        case MIECZ:
        case KARABIN:
            while (2>1){
                int zegar=getZegar(0);
                pthread_mutex_unlock(bron_zgoda_mut(wybranaBron));
                //TODO - potwierdzic ze dobrze
                // jezeli akurat nie chce broni (ani nie poluje ta bronia, ale to ogarnia muteks)
                //lub chce inna bron
                //lub ziomek ma nizszy zegar albo taki sam ale nizszy rank
                if ((getStan()!=JO_CHCA)||(juzWybralemBron==TRUE&&wybranaBron!=pakiet->tresc)||pakiet->ts<zegar||(pakiet->ts==zegar&&rank>pakiet->src)){
                    packet_t tmp;
                    tmp.tresc = pakiet->tresc; 
                    tmp.ts=getZegar(1);
                    tmp.id=pakiet->id;
                    tmp.reszta=JO_CHCA;
                    sendPacket(&tmp, pakiet->src, JO_CHCA);
                    pthread_mutex_unlock(bron_zgoda_mut(wybranaBron));
                    break;
                }
                ifnt{
                    pthread_mutex_unlock(bron_zgoda_mut(wybranaBron));
                    //jakis wait???
                }
            }
            break;
        case MEDYK:
            while (2>1){
            
            int zegar=getZegar(0);
            pthread_mutex_lock(&sanitariusz_zgoda_mut);
            //TODO - potwierdzic ze dobrze
            // jezeli akurat nie jestem ranny (ani leczony, ale to ogarnia muteks)
            //lub ziomek ma nizszy zegar albo taki sam ale nizszy rank
            if ((getStan()!=RANNY)||pakiet->ts<zegar||(pakiet->ts==zegar&&rank>pakiet->src)){
                packet_t tmp;
                tmp.tresc = pakiet->tresc; 
                tmp.ts=getZegar(1);
                tmp.id=pakiet->id;
                tmp.reszta=RANNY;
                sendPacket(&tmp, pakiet->src, JO_CHCA);
                pthread_mutex_unlock(&sanitariusz_zgoda_mut);
                break;
            }
            ifnt{
                pthread_mutex_unlock(&sanitariusz_zgoda_mut);
                //jakis wait???
            }
            }
            break;
    }
    
}

void pozwolenieHandler( packet_t *pakiet)
{
    int stanTmp=getStan();
    int oIle=0;
    if(pakiet->id==idOstatiegoRequesta&&stan==pakiet->reszta){
        oIle=1;
    }
    int tmp=zwiekszRzeczzMuteksami(&otrzymaneZgody,&zgoda_mut,oIle);
    switch (stanTmp){
        case CHCE_BRON:
            if(juzWybralemBron==TRUE){
                if((wybranaBron==MIECZ&&tmp>=size-LICZBA_MIECZY)||(wybranaBron==KARABIN&&tmp>=size-LICZBA_KARABINOW)){
                    zerujZgody();
                    pthread_mutex_unlock(&zgoda_mut);
                }
            }
            break;
        case RANNY:
            if (tmp>=size-LICZBA_SANITARIUSZY){
                zerujZgody();
                pthread_mutex_unlock(&zgoda_mut);
            }            
            break;
        case CHCE_MIEJSCE_W_WYPYCHALNI:
        //TODO warunek
            if(2<1){
                zerujZgody();
                pthread_mutex_unlock(&zgoda_mut); 
            }
            break;      
    }
}

void zgonHandler( packet_t *pakiet)
{
    zwiekszRzeczzMuteksami(&liczbaMartwychLowcow,&martwi_mut,1);
    //TODO
}

void placeholderHandler( packet_t *pakiet)
{
    //XD
}

void bandersnatchowniaHandler( packet_t *pakiet)
{
    int tmp;
    //4 opcje - tresc to ktora wiadomosc
    // reszta to rozmiar
    //CHCE
    //ZGADAM SIE (odp na chce)
    //ZAJMUJE
    //ZWALNIAM
    switch(pakiet->tresc){
        case CHCE:
            while (2>1){
            
            int zegar=getZegar(0);
            pthread_mutex_lock(&wypychalnia_zgoda_mut);
            //TODO - potwierdzic ze dobrze
            //zasady jak zawsze
            if ((getStan()!=CHCE_MIEJSCE_W_WYPYCHALNI)||pakiet->ts<zegar||(pakiet->ts==zegar&&rank>pakiet->src)){
                packet_t tmp;
                tmp.tresc = POZWALAM; 
                tmp.ts=getZegar(1);
                tmp.id=pakiet->id;
                sendPacket(&tmp, pakiet->src, BANDERSNACHOWNIA);
                pthread_mutex_lock(&wypychalnia_zgoda_mut);
                break;
            }
            ifnt{
                pthread_mutex_lock(&wypychalnia_zgoda_mut);
                //jakis wait???
            }
            }
            break;
        //otrzymanie zgody
        //TODO check id
        case POZWALAM:
            tmp=zwiekszRzeczzMuteksami(&potencjalnieZajeteMiejsce,&potencjalne_miejsce_w_wypychalni_mut,-1*MAX_ROZMIAR_BANDERSNATCHA);
            if(tmp<=rozmiarMojegoBandersnatcha)
            {
                zerujZgody();
                pthread_mutex_unlock(&zgoda_mut); 
            }
            break;
        case ZAJMUJE:
            zwiekszRzeczzMuteksami(&miejsceNieZajetePrzezBedacychwWypychalni,&miejsce_niezajete_w_wypychalni_mut,MAX_ROZMIAR_BANDERSNATCHA-pakiet->reszta);
            break;
        case ODDAJE:
            zwiekszRzeczzMuteksami(&miejsceNieZajetePrzezBedacychwWypychalni,&miejsce_niezajete_w_wypychalni_mut,-(MAX_ROZMIAR_BANDERSNATCHA-pakiet->reszta));
            break;
    };
    }
