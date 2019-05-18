#include "main.h"
#include <time.h>
MPI_Datatype MPI_PAKIET_T;
pthread_t threadCom, threadM;

/* zamki do synchronizacji zmiennych współdzielonych */
pthread_mutex_t zegar_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t otrzymane_zgody_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t currentId_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stan_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t potencjalne_miejsce_w_wypychalni_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t miejsce_niezajete_w_wypychalni_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t martwi_mut = PTHREAD_MUTEX_INITIALIZER;


//muteksy zgadzania sie

pthread_mutex_t sanitariusz_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wypychalnia_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;

//broniowe
pthread_mutex_t miecz_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t karabin_zgoda_mut = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t end_odbior_mut = PTHREAD_MUTEX_INITIALIZER;
sem_t all_sem;

int zegarLamporta = 0;
int stan = CHCE_BRON;

int zegarOstatniegoRequesta = 0;
int idOstatiegoRequesta = 0;
int otrzymaneZgody = 0;

volatile int wybranaBron = -1;

volatile int rozmiarMojegoBandersnatcha = -1;

int potencjalnieZajeteMiejsce = 0;

int liczbaMartwychLowcow = 0;

int miejsceNieZajetePrzezBedacychwWypychalni = 0;

char zasoby[3][8] = {
    [MIECZ] = "  MIECZ",
    [KARABIN] = "KARABIN",
    [MEDYK] = "   MEDYK"};
int LICZBA_ZASOBOW[3] = {
    [MIECZ] = LICZBA_MIECZY,
    [KARABIN] = LICZBA_KARABINOW,
    [MEDYK] = LICZBA_SANITARIUSZY};

/* end == TRUE oznacza wyjście z main_loop */
volatile bool end = FALSE;
volatile bool zyje = TRUE;
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

f_w handlers[MAX_HANDLERS] = {[JO_CHCA] = joChcaHandler,
                              [POZWALAM] = pozwolenieHandler,
                              [UMIERAM] = zgonHandler,
                              [PLACEHOLDER] = placeholderHandler,
                              [BANDERSNACHOWNIA] = bandersnatchowniaHandler};

extern void inicjuj(int *argc, char ***argv);
extern void finalizuj(void);

pthread_mutex_t *bron_zgoda_mut(int co)
{
    if (co == MIECZ)
        return &miecz_zgoda_mut;
    if (co == KARABIN)
        return &karabin_zgoda_mut;
    return &miecz_zgoda_mut;
}

//TODO - przetestowac
int ustawRzeczzMuteksami(int *co, pthread_mutex_t *muteks, int naIle)
{
    int a = -1;
    pthread_mutex_lock(muteks);
    *co = naIle;
    a = *co;
    pthread_mutex_unlock(muteks);
    return a;
}

//TODO - przetestowac
int zwiekszRzeczzMuteksami(int *co, pthread_mutex_t *muteks, int oIle)
{
    int a = -1;
    pthread_mutex_lock(muteks);
    *co += oIle;
    a = *co;
    pthread_mutex_unlock(muteks);

    return a;
}

int getZegar(int inkrementowac)
{
    int a = -1;
    pthread_mutex_lock(&zegar_mut);
    zegarLamporta += inkrementowac;
    a = zegarLamporta;
    pthread_mutex_unlock(&zegar_mut);
    return a;
}

int getStan()
{
    int a;
    pthread_mutex_lock(&stan_mut);
    a = stan;
    pthread_mutex_unlock(&stan_mut);
    return a;
}

void zerujZgody()
{
    pthread_mutex_lock(&otrzymane_zgody_mut);
    otrzymaneZgody = 0;
    pthread_mutex_unlock(&otrzymane_zgody_mut);
}

void zmienStan(int naCo)
{
    pthread_mutex_lock(&stan_mut);
    stan = naCo;
    pthread_mutex_unlock(&stan_mut);
}

int setZegarToMax(int new)
{
    int a = -1;
    pthread_mutex_lock(&zegar_mut);
    if (new > zegarLamporta)
        zegarLamporta = new;
    zegarLamporta++;
    a = zegarLamporta;
    pthread_mutex_unlock(&zegar_mut);
    return a;
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc, &argv);

    mainLoop();

    finalizuj();
    return 0;
}

float myRandomFloat()
{
    return (float)((float)rand() / (float)RAND_MAX);
}

void czekajNaZgode()
{
    pthread_mutex_lock(&zgoda_mut);
    pthread_mutex_unlock(&zgoda_mut);
}

bool czyTrzebaWysylacZgody()
{
    int deadBois = zwiekszRzeczzMuteksami(&liczbaMartwychLowcow, &martwi_mut, 0);
    int zasob = wybranaBron;
    if (zasob < 0)
    {
        zasob = MEDYK;
    }

    if (LICZBA_ZASOBOW[zasob] >= size - deadBois){
        println("PONIEWAZ %s JEST %d, A JEST NAS ZYWYCH %d, TO NIE WYSYLAM PYTAN\n", zasoby[zasob], LICZBA_ZASOBOW[zasob], size - deadBois);
        return FALSE;
    }
    
    return TRUE;
}

void OurBroadcast(int tresc, int typ, int reszta)
{
    if (typ == JO_CHCA)
    {
        if (!czyTrzebaWysylacZgody())
        {
            pthread_mutex_unlock(&zgoda_mut);
            return;
        }
    }
    zerujZgody();
    idOstatiegoRequesta++;
    zegarOstatniegoRequesta = getZegar(1);
    packet_t tmp;
    tmp.tresc = tresc;
    tmp.ts = zegarOstatniegoRequesta;
    tmp.id = idOstatiegoRequesta;
    tmp.reszta = reszta;
    for (int i = 0; i < size; i++)
    {
        if (rank != i)
        {
            if (typ == JO_CHCA)
            {
                if (wybranaBron > -1)
                {
                    println("WYSYLAM PROSBE O %s DO %d\n", zasoby[wybranaBron], i);
                }
                ifnt
                {
                    println("WYSYLAM PROSBE O MEDYKA DO %d\n", i);
                }
            }
            if (typ == BANDERSNACHOWNIA)
            {
                if (tresc == CHCE)
                    println("WYSYLAM PROSBE O MIEJSCE W WYPYCHALNI DO %d\n", i);
                if (tresc == ZAJMUJE)
                    println("WYSYLAM INFO, ZE ZAJMUJE MIEJSCE W WYPYCHALNI DO %d\n", i);
                if (tresc == ODDAJE)
                    println("WYSYLAM INFO, ZE ODDAJE MIEJSCE W WYPYCHALNI DO %d\n", i);
            }
            if (typ == UMIERAM)
            {
                println("WYSYLAM INFO, ZE ZGONUJE DO %d\n", i);
            }
            sendPacket(&tmp, i, typ);
        }
    }
}

void mainLoop(void)
{
    int iter;
    srand(time(NULL));

    /* mały sleep, by procesy nie zaczynały w dokładnie tym samym czasie */
    struct timespec t = {0, rank * 50000};
    struct timespec rem = {1, 0};
    nanosleep(&t, &rem);
    //

    // stan = CHCE_MIEJSCE_W_WYPYCHALNI;
    /* pętla główna: sen, wysyłanie przelewów innym bankom */
    pthread_mutex_lock(&end_odbior_mut);
    int ttl=0;
    while (zyje)
    {
        int percent = rand() % 2 + 1;
        struct timespec t = {percent, 0};
        struct timespec rem = {1, 0};
        nanosleep(&t, &rem);
        switch (stan)
        {
        case CHCE_BRON:
            // wybranaBron = (int)(myRandomFloat() * LICZBA_TYPOW_BRONI);
            wybranaBron = MIECZ;
            println("WYBRALEM %s\n", zasoby[wybranaBron]);
            pthread_mutex_lock(&zgoda_mut);
            OurBroadcast(wybranaBron, JO_CHCA, 0);
            czekajNaZgode();
            println("DOSTALEM %s, ZACZYNAM POLOWAC\n", zasoby[wybranaBron]);
            pthread_mutex_lock(bron_zgoda_mut(wybranaBron));
            zmienStan(POLUJE);
            break;
        case POLUJE:
            iter=CZAS_POLOWANIA;
            while(iter-->0){
                sleep(1);
                println("POLUJE\n");
            }
            pthread_mutex_unlock(bron_zgoda_mut(wybranaBron));
            println("SKONCZYLEM POLOWAC %sEM\n", zasoby[wybranaBron]);
            wybranaBron = -1;
            // sleep(1);
            // zmienStan(RANNY);
            // break;
            int r = (int)(myRandomFloat() * 100);
            if (ttl--<=0)
                zmienStan(MARTWY);
            ifnt
            {
                zmienStan(RANNY);
            }
            // if(r<SZANSA_ZGONU){
            //     zmienStan(MARTWY);
            // }
            // ifnt{
            //     r-=SZANSA_ZGONU;
            //     if(r<SZANSA_RANNOSCI){
            //         zmienStan(RANNY);
            //     }
            //     ifnt{
            //         zmienStan(CHCE_MIEJSCE_W_WYPYCHALNI);
            //     }
            // }
            break;
        case MARTWY:
            OurBroadcast(rank, UMIERAM, 0);
            sleep(1);
            zyje=FALSE;
            pthread_mutex_lock(&end_odbior_mut);
            break;
        case RANNY:
            pthread_mutex_lock(&zgoda_mut);
            OurBroadcast(MEDYK, JO_CHCA, 0);
            czekajNaZgode();
            pthread_mutex_lock(&sanitariusz_zgoda_mut);
            zmienStan(LECZONY);
            break;
        case CHCE_MIEJSCE_W_WYPYCHALNI:
            pthread_mutex_lock(&zgoda_mut);
            rozmiarMojegoBandersnatcha = (int)myRandomFloat() * MAX_ROZMIAR_BANDERSNATCHA + 1;
            // rozmiarMojegoBandersnatcha = rank*2+1;
            //to jest rozmiarMojegoBandersnatchazle
            //ustawRzeczzMuteksami(miejsceNieZajetePrzezBedacychwWypychalni,miejsce_niezajete_w_wypychalni_mut,MAX_ROZMIAR_BANDERSNATCHA*(size-zwiekszRzeczzMuteksami(liczbaMartwychLowcow,martwi_mut,0)));
            int liczbaZyjacychBezeMnie = size - zwiekszRzeczzMuteksami(&liczbaMartwychLowcow, &martwi_mut, 0) - 1;
            ustawRzeczzMuteksami(&potencjalnieZajeteMiejsce, &potencjalne_miejsce_w_wypychalni_mut, MAX_ROZMIAR_BANDERSNATCHA * liczbaZyjacychBezeMnie);
            println("WYSYLAM PROSBE O MIEJSCE W WYPYCHALNI %d\n", rozmiarMojegoBandersnatcha);
            OurBroadcast(CHCE, BANDERSNACHOWNIA, rozmiarMojegoBandersnatcha);
            czekajNaZgode();
            pthread_mutex_lock(&wypychalnia_zgoda_mut);
            println("ZAJMUJE MIEJSCE W WYPYCHALNI %d\n", rozmiarMojegoBandersnatcha);
            OurBroadcast(ZAJMUJE, BANDERSNACHOWNIA, rozmiarMojegoBandersnatcha);
            zmienStan(SIEDZE_W_WYPYCHALNI);
            break;
        case SIEDZE_W_WYPYCHALNI:
            // println("DEBUG, WLAZLEM\n");
            iter=CZAS_WYPYCHANIA;
            while(iter-->0){
                sleep(1);
                println("WYPYCHAM\n");
            }
            // println("DEBUG, SKONCZYLEM\n");
            pthread_mutex_unlock(&wypychalnia_zgoda_mut);

            OurBroadcast(ODDAJE, BANDERSNACHOWNIA, rozmiarMojegoBandersnatcha);
            println("ZWALNIAM MIEJSCE W WYPYCHALNI %d\n", rozmiarMojegoBandersnatcha);
            rozmiarMojegoBandersnatcha = -1;
            zmienStan(JO_CHCA);
            end = TRUE;
            break;
        case LECZONY:
            println("ROZPOCZYNAM LECZENIE\n");
            iter=CZAS_LECZENIA;
            while(iter-->0){
                sleep(1);
                println("LECZE SIE\n");
            }
            pthread_mutex_unlock(&sanitariusz_zgoda_mut);
            println("KONCZE LECZENIE\n");
            zmienStan(CHCE_BRON);
            break;
        }
    }
    println(" Koniec zycia! \n");    
}

/* Wątek komunikacyjny - dla każdej otrzymanej wiadomości wywołuje jej handler */
void *comFunc(void *ptr)
{
    MPI_Status status;
    /* odbieranie wiadomości */
    while (!end)
    {
        packet_t *pakiet = (packet_t *)malloc(sizeof(packet_t));
        // println("[%d] czeka na recv\n", rank);
        MPI_Recv(pakiet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        println("%d\n",status.MPI_TAG);
        if (!zyje && (int)status.MPI_TAG != UMIERAM)
        {
            free(pakiet);
            continue;
        }
        pakiet->src = status.MPI_SOURCE;
        setZegarToMax(pakiet->ts);
        // println("odebrlaem cos od %d o tresci %d\n",pakiet.src,pakiet.tresc); //DEBUG
        // handlers[(int)status.MPI_TAG](&pakiet); // zamiast wielkiego switch status.MPI_TAG... case X: handler()
        if (status.MPI_TAG != UMIERAM){
        pthread_t new_thread;
        pthread_create(&new_thread, NULL, (void *)handlers[(int)status.MPI_TAG], pakiet);
        }
        ifnt{
            int deadBois = zwiekszRzeczzMuteksami(&liczbaMartwychLowcow, &martwi_mut, 1);
            println("DOSTALEM INFO O ZGONIE OD %d, LICZBA ZGONOW %d\n",pakiet->src,deadBois);
            if (deadBois >= size - 1){
                println("END\n");
                end = TRUE;
            }
                
            free(pakiet);
        }
    }
    pthread_mutex_unlock(&end_odbior_mut);
    println(" Koniec komunikacji! \n");
    return 0;
}

/* Handlery */
void joChcaHandler(packet_t *pakiet)
{
    // printf("odebraem JOCHCA\n"); //DEBUG
    println("ROZPATRUJE ZGODE NA %s DLA %d\n", zasoby[pakiet->tresc], pakiet->src);
    bool wyslalem = FALSE;
    switch (pakiet->tresc)
    {
    case MIECZ:
    case KARABIN:
        while (!wyslalem)
        {
            pthread_mutex_lock(bron_zgoda_mut(pakiet->tresc));
            //TODO - potwierdzic ze dobrze // no nie
            // jezeli akurat nie chce broni (ani nie poluje ta bronia, ale to ogarnia muteks)
            //lub chce inna bron
            //lub ziomek ma nizszy zegar albo taki sam ale nizszy rank
            // printf("%d %d %d %d\n",pakiet->ts,zegarOstatniegoRequesta,rank,pakiet->src);
            int zegarPozwala = (pakiet->ts < zegarOstatniegoRequesta || (pakiet->ts == zegarOstatniegoRequesta && rank > pakiet->src));
            if ((wybranaBron != pakiet->tresc) || zegarPozwala)
            {

                packet_t tmp;
                tmp.tresc = pakiet->tresc;
                tmp.tresc = pakiet->tresc;
                tmp.tresc = pakiet->tresc;
                tmp.ts = getZegar(1);
                tmp.id = pakiet->id;
                tmp.reszta = CHCE_BRON;
                println("WYSYLAM ZGODE NA %s DO %d\n", zasoby[pakiet->tresc], pakiet->src);
                sendPacket(&tmp, pakiet->src, POZWALAM);
                pthread_mutex_unlock(bron_zgoda_mut(pakiet->tresc));
                wyslalem = TRUE;
            }
            ifnt
            {
                pthread_mutex_unlock(bron_zgoda_mut(pakiet->tresc));
                if (end)
                    break;
            }
        }
        break;
    case MEDYK:
        while (!wyslalem)
        {
            pthread_mutex_lock(&sanitariusz_zgoda_mut);
            //TODO - potwierdzic ze dobrze
            // jezeli akurat nie jestem ranny (ani leczony, ale to ogarnia muteks)
            //lub ziomek ma nizszy zegar albo taki sam ale nizszy rank
            int zegarPozwala = (pakiet->ts < zegarOstatniegoRequesta || (pakiet->ts == zegarOstatniegoRequesta && rank > pakiet->src));
            if ((getStan() != RANNY) || zegarPozwala)
            {
                packet_t tmp;
                tmp.tresc = pakiet->tresc;
                tmp.ts = getZegar(1);
                tmp.id = pakiet->id;
                tmp.reszta = RANNY;
                println("WYSYLAM ZGODE NA MEDYKA DO %d\n", pakiet->src);
                sendPacket(&tmp, pakiet->src, POZWALAM);
                pthread_mutex_unlock(&sanitariusz_zgoda_mut);
                wyslalem = TRUE;
            }
            ifnt
            {
                pthread_mutex_unlock(&sanitariusz_zgoda_mut);
                if (end)
                    break;
            }
        }
        break;
    }
    free(pakiet);
}

void pozwolenieHandler(packet_t *pakiet)
{

    int stanTmp = getStan();

    // printf("%d %d %d %d\n",pakiet->id,idOstatiegoRequesta,stan,pakiet->reszta);
    if (pakiet->id == idOstatiegoRequesta && stan == pakiet->reszta)
    {
        int deadBois = zwiekszRzeczzMuteksami(&liczbaMartwychLowcow, &martwi_mut, 0);
        int tmp = zwiekszRzeczzMuteksami(&otrzymaneZgody, &otrzymane_zgody_mut, 1);
        println("DOSTALEM ZGODE NA %s OD %d, MAM %d ZGOD\n", zasoby[pakiet->tresc], pakiet->src, tmp);
        switch (stanTmp)
        {
        case CHCE_BRON:
            if (wybranaBron >= 0)
            {
                if (tmp >= size - deadBois - LICZBA_ZASOBOW[wybranaBron])
                {
                    zerujZgody();
                    pthread_mutex_unlock(&zgoda_mut);
                }
            }
            break;
        case RANNY:
            if (tmp >= size - deadBois - LICZBA_SANITARIUSZY)
            {
                zerujZgody();
                pthread_mutex_unlock(&zgoda_mut);
            }
            break;
        }
    }
    ifnt
    {
        println("DOSTALEM ZGODE NA %s OD %d, ALE JUZ NVM\n", zasoby[pakiet->tresc], pakiet->src);
    }
    free(pakiet);
}

void zgonHandler(packet_t *pakiet)
{
    int deadBois = zwiekszRzeczzMuteksami(&liczbaMartwychLowcow, &martwi_mut, 1);
    println("DOSTALEM INFO O ZGONIE OD %d, LICZBA ZGONOW %d\n",pakiet->src,deadBois);
    if (deadBois >= size - 1){
        end = TRUE;
    }
        
    free(pakiet);
}

void placeholderHandler(packet_t *pakiet)
{
    //XD
    free(pakiet);
}

void bandersnatchowniaHandler(packet_t *pakiet)
{
    int temp, a, b;
    //4 opcje - tresc to ktora wiadomosc
    // reszta to rozmiar
    //CHCE
    //ZGADAM SIE (odp na chce)
    //ZAJMUJE
    //ZWALNIAM
    switch (pakiet->tresc)
    {
    case CHCE:
        println("ROZPATRUJE POZWOLENIE NA ZAJECIE %d MIEJSCA W WYPYCHALNI DO %d\n", pakiet->reszta, pakiet->src);
        while (2 > 1)
        {
            pthread_mutex_lock(&wypychalnia_zgoda_mut);
            int zegar = zegarOstatniegoRequesta;

            //TODO - potwierdzic ze dobrze
            //zasady jak zawsze
            bool nieRobieNicPrzyBandersnatchowni = rozmiarMojegoBandersnatcha < 0;
            bool jeszczeNieWszedlem = getStan() != SIEDZE_W_WYPYCHALNI;
            bool zegarPozwala = jeszczeNieWszedlem && (pakiet->ts < zegar || (pakiet->ts == zegar && rank > pakiet->src));
            if (nieRobieNicPrzyBandersnatchowni || zegarPozwala)
            {
                println("WYSYLAM ZGODE NA ZAJECIE %d MIEJSCA W WYPYCHALNI DO %d\n", pakiet->reszta, pakiet->src);
                packet_t tmp;
                tmp.tresc = POZWALAM;
                tmp.ts = getZegar(1);
                tmp.id = pakiet->id;
                sendPacket(&tmp, pakiet->src, BANDERSNACHOWNIA);
                pthread_mutex_unlock(&wypychalnia_zgoda_mut);
                break;
            }
            ifnt
            {
                pthread_mutex_unlock(&wypychalnia_zgoda_mut);
                //jakis wait???
            }
        }
        break;
    case POZWALAM:
        // println ("DEBUG, JAKAS ZGODA %d, %d, %d\n",pakiet->id,idOstatiegoRequesta,stan);
        //otrzymanie zgody
        if (pakiet->id == idOstatiegoRequesta && stan == CHCE_MIEJSCE_W_WYPYCHALNI)
        {
            temp = zwiekszRzeczzMuteksami(&potencjalnieZajeteMiejsce, &potencjalne_miejsce_w_wypychalni_mut, -MAX_ROZMIAR_BANDERSNATCHA) - zwiekszRzeczzMuteksami(&miejsceNieZajetePrzezBedacychwWypychalni, &miejsce_niezajete_w_wypychalni_mut, 0);
            println("DOSTALEM ZGODE NA ZAJECIE MIEJSCA W WYPYCHALNI OD %d, MOJ BANDERSNATCH MA %d, POTENCJALNIE ZAJETE JEST %d z %d MIEJSCA\n", pakiet->src, rozmiarMojegoBandersnatcha, temp, ILOSC_MIEJSCA_W_WYPYCHALNI);
            int coNajmniejWolneJest = MAX_ROZMIAR_BANDERSNATCHA - temp;
            if (coNajmniejWolneJest >= rozmiarMojegoBandersnatcha)
            {
                zerujZgody();
                pthread_mutex_unlock(&zgoda_mut);
            }
        }
        ifnt
        {
            println("DOSTALEM ZGODE NA ZAJECIE MIEJSCA W WYPYCHALNI OD %d, ALE JUZ NVM\n", pakiet->src);
            if (end)
                break;
        }
        break;
    case ZAJMUJE:
        a = zwiekszRzeczzMuteksami(&miejsceNieZajetePrzezBedacychwWypychalni, &miejsce_niezajete_w_wypychalni_mut, MAX_ROZMIAR_BANDERSNATCHA - pakiet->reszta);
        println("DOSTALEM INFO ZE %d ZAJMUJE %d MIEJSCA W WYPYCHALNI, MIEJSCE ZOSTAWIONE PRZEZ WYPYCHAJACYCH TO %d\n", pakiet->src, pakiet->reszta, a);
        break;
    case ODDAJE:
        b = zwiekszRzeczzMuteksami(&miejsceNieZajetePrzezBedacychwWypychalni, &miejsce_niezajete_w_wypychalni_mut, -(MAX_ROZMIAR_BANDERSNATCHA - pakiet->reszta));
        println("DOSTALEM INFO ZE %d ODDAJE %d MIEJSCA W WYPYCHALNI, MIEJSCE ZOSTAWIONE PRZEZ WYPYCHAJACYCH TO %d\n", pakiet->src, pakiet->reszta, b);
        break;
    }
    free(pakiet);
}