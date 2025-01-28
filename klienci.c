#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include "semafor.h"
#include <math.h>
#include <stdarg.h>

#define MAX_KLIENCI 10
#define MAX_POCZEKALNIA 6
struct shared_memory {
    pid_t pid_klienta;  
    pid_t pid_kasjera;  
    pid_t pid_fryzjera;
    int kwota;           
    int zaplacone;       
    int reszta[3];          
    int banknoty[3];     
};
#define SHM_SIZE sizeof(struct shared_memory)
struct msgbuf {
    long mtype;  
    pid_t pid; 
    int id;
};
struct msgbuf2 {
    long mtype;  
    pid_t pid; 
    int liczba;
};
struct sembuf sb;
 volatile int emergency_closeup=20;
 volatile int semafor_id;

void zapisz(const char *format, ...) {
    FILE *file = fopen("klienci_summary.txt", "a");
    if (file == NULL) {
        perror("Błąd przy otwieraniu pliku");
        return;
    }

    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsprintf(buffer, format, args);
    printf("%s", buffer);
    fprintf(file, "%s", buffer);

    va_end(args);
    fclose(file);
}


int suma(int *portfel)
{
    int sumuj=portfel[0] * 10 + portfel[1] * 20 + portfel[2] * 50;
    return sumuj;
}

void przelicz_pieniadze(int bin,int *banknoty)
{
    for(int i=0;i<2;i++)
    { 
        banknoty[i]=bin%10;
    bin=bin/10;
    }
}

int obsluz_signal2(int sig, siginfo_t *info,void *ucontext) {
    if (sig == SIGUSR1) {
        //printf("klient: Otrzymał inta %d\n",info->si_value.sival_int);
        kill(info->si_value.sival_int, SIGUSR1) == -1;//synchronizacja  
    }
    else
    {
        zapisz("BLAD Sygnalu:v2\n");
    }
    return 0;
}
int obudz(int sig) {
    if (sig == SIGUSR1) {
        zapisz("%d dostal sygnal\n",getpid());
    }
    else
    {
        zapisz("BLAD Sygnalu:\n");
    }
    return 0;
}
int obudz2(int sig) {
    if (sig == SIGUSR1) {
        zapisz("%d dostal sygnal od kasjera\n",getpid());
        printf("%d: tak wyszedlem\n",getpid());
    }
    else
    {
        zapisz("BLAD Sygnalu:\n");
    }
    return 0;
}
void zakocz_sie(int sig) {
    if (sig == SIGUSR2) {
        emergency_closeup=0;
    zapisz("klient %d skonczyl dzialanie\n",getpid());
    exit(0);
    }
    else
    {
        zapisz("BLAD Sygnalu:\n");
    }
}
/*void zakocz_sie_z_sem(int sig) {
    if (sig == SIGUSR2) {
        struct sembuf sb;
        sb.sem_flg = 0;
        sb.sem_op = 1;  
        sb.sem_num = 0;
        semop(semafor_id, &sb, 1);
        emergency_closeup=0;
    zapisz("klient %d skonczyl dzialanie\n",getpid());
    exit(0);
    }
    else
    {
        zapisz("BLAD Sygnalu:\n");
    }
}*/

void zarabiaj(int* portfel)
{
    int liczba = rand() % 3 + 1;
    sleep(1);
    if (liczba >= 0 && liczba <= 2) {
        portfel[liczba]++;
        zarabiaj(portfel);
    }
    else if (suma(portfel) < 100) zarabiaj(portfel);
}

int zajmij_miejsce(int id, int semid)
{
    if (semctl(semid, 0, GETVAL) > 0)
    {
        sb.sem_num = 0;
        sb.sem_op = -1; 
        sb.sem_flg = 0;
        semop(semid, &sb, 1);
        semafor_id=semid;
       // signal(SIGUSR2, zakocz_sie_z_sem);
        zapisz("klient %d: Zajął miejsce w poczekalni.(%d/%d)\n", id, semctl(semid, 0, GETVAL), MAX_POCZEKALNIA);
        return 1;
    }
    else
    {
        zapisz("klient %d: Nie było miejsca dla klienta wiec wychodzi.\n", id);
        return 0;
    }

}

void czekaj_na_fryzjera(int id,int msgid)
{
struct msgbuf message;
    message.mtype = 1;  
    message.pid = getpid();
    message.id=id;

if (msgsnd(msgid, &message, sizeof(struct msgbuf), 0) == -1) 
    {
        perror("Błąd przy wysyłaniu wiadomości");
        exit(1);
    }
    signal(SIGUSR1, obudz);
      zapisz("klient %d: wyslal swoj pid[%d] do kolejki\n",id,getpid());
    //signal(SIGUSR1, SIG_IGN);  
}


void usiadz_na_fotelu(int id)
{ 
    pause();
    zapisz("klient: otrzymal sygnal, idzie do fotela.\n");
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = obsluz_signal2;  
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Błąd przy rejestracji sygnału");
        exit(1);
    }
    zapisz("klient %d usiadl na fotelu\n",id);
}



void zaplac(int id,int *portfel,int pamiec_kasjer,int semafor_kasjer,int kolejka_klienci2)
{
    zapisz("klient %d chce zaplacic\n",id);
    struct msgbuf message;
    message.mtype = 1;  
    message.pid = getpid();
    message.id=id;
    signal(SIGUSR1, obudz2);
        sb.sem_num = 0;
        sb.sem_op = -1; 
        sb.sem_flg = 0;
    zapisz("klient %d czeka na kasjera \n",id);  
        
        printf("stan semafora:%d\n",semctl(semafor_kasjer,0,GETVAL));
        semop(semafor_kasjer, &sb, 1);
        printf("stan semafora:%d\n",semctl(semafor_kasjer,0,GETVAL));

        struct shared_memory *shared_mem = (struct shared_memory *)shmat(pamiec_kasjer, NULL, 0);
        if(shared_mem == -1) {
        perror("Błąd przy przypisaniu pamięci współdzielonej");
        exit(1);
        }
        zapisz("klient %d: budzi kasjera\n",id);
        shared_mem->pid_klienta = getpid();
    if (msgsnd(kolejka_klienci2, &message, sizeof(struct msgbuf), 0) == -1) 
    {
        perror("Błąd przy wysyłaniu wiadomości");
        exit(1);
    }
    pause();
    zapisz("klient%d:wyciaga portfel\n",id);
        int nominaly[3] = { 10,20,50 };
        int zaplacone = 0;
        zapisz("klient %d:szuka pieniedzy w portfelu by zaplacic %d\n",id,shared_mem->kwota);
        zapisz("klient %d ma: %dx10zl, %dx20zl, %dx50zl\n",id,portfel[0],portfel[1],portfel[2]);
    for (int i = 2; i>=0; i--)
    {
        if(zaplacone < shared_mem->kwota&&portfel[i]>0)
        { 
        portfel[i]--;
        shared_mem->banknoty[i]++;
        zaplacone = zaplacone + nominaly[i];
        i++;
        }
    }
    zapisz("klient %d:placi%d z %d\n",id,zaplacone,shared_mem->kwota);
    if(kill(shared_mem->pid_kasjera, SIGUSR1) == -1){//zaplac
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    }
    pause();

    zapisz("klient %d: dostaje reszte\n",id);
    for(int i=0;i<=2;i++)
    { 
        portfel[i]+=shared_mem->reszta[i];
    }
    zapisz("klient %d: po wydaniu reszty ma %dx10zl, %dx20zl, %dx50zl\n",id,portfel[0],portfel[1],portfel[2]);
    if(kill(shared_mem->pid_fryzjera, SIGUSR1) == -1){
        perror("Błąd przy wysyłaniu sygnału1");
        exit(1);
    };
     if(kill(shared_mem->pid_kasjera, SIGUSR1) == -1){
        perror("Błąd przy wysyłaniu sygnału2");
        exit(1);
    };
        sb.sem_num = 0;
        sb.sem_op = 1; 
        sb.sem_flg = 0;
        semop(semafor_kasjer, &sb, 1);
        signal(SIGUSR1, obudz);
      kill(shared_mem->pid_fryzjera,SIGUSR1);  
      zapisz("klient %d: wysyla sygnal do zniecierpliwionego fryzjera\n",id); 
}


void zostan_ostrzyzonym(int id)
{
    pause();
    zapisz("Klient %d: schodzi z fotela zadowolony\n",id);
}

void opusc_salon(int id, int semid)
{
    sb.sem_op = 1;  
    semop(semid, &sb, 1);
    signal(SIGUSR2, zakocz_sie);
    zapisz("Klient %d: Opuszcza poczekalnię.(%d/%d)\n", id, semctl(semid, 0, GETVAL), MAX_POCZEKALNIA);
}

void klient(int id, int semid,int msgid,int pamiec_kasjer,int semafor_kasjer,int kolejka_klienci2) {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = zakocz_sie; 
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd przy ustawianiu obsługi SIGUSR2");
        exit(1);
    }
    zapisz("klient %d stworzony [%d]\n",id,getpid());

 int *portfel =(int*) malloc(3*sizeof(int));
    for (int i = 0;i < 3;i++) portfel[i] = 0;
 while(emergency_closeup){
        int temp_money=suma(portfel);
        zapisz("Klient %d: Idzie zarabiać.\n",id);
        zarabiaj(portfel);
        zapisz("Klient %d: zarobił %dzł.[%d]->[%d]\n",id,suma(portfel)-temp_money,temp_money,suma(portfel));
        sleep(1);
        if(zajmij_miejsce(id, semid))
        {
        czekaj_na_fryzjera(id,msgid);
        usiadz_na_fotelu(id);
        temp_money=suma(portfel);
        zaplac(id,portfel,pamiec_kasjer,semafor_kasjer,kolejka_klienci2);
        zapisz("Klient %d: zaplacil [%d]->[%d]\n",id,temp_money,suma(portfel));
        zostan_ostrzyzonym(id);
        opusc_salon(id, semid);
        
        }
    emergency_closeup--;
    }
  
}


void generuj_klientow(key_t key,int msgid,int pamiec_kasjer,int semafor_kasjer,int kolejka_klienci,int kolejka_klienci2) {
    int semid = utworz_semafor(key, 1);  
    ustaw_semafor(semid, 0, MAX_POCZEKALNIA);
    for (int i = 0; i < MAX_KLIENCI; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            struct msgbuf message;
               message.mtype = 1;  
                 message.pid = getpid();
                 message.id=i+1;
            if (msgsnd(kolejka_klienci, &message, sizeof(struct msgbuf), 0) == -1) 
                 {
                     perror("Błąd przy dodawaniu pida do kolejki klienci");
                     exit(1);
                 }
            klient(i + 1, semid,msgid,pamiec_kasjer,semafor_kasjer,kolejka_klienci2);
            exit(0);
        }
        else if (pid < 0) {
            perror("Błąd przy tworzeniu procesu");
            exit(1);
        }


        //int czas = rand() % 5 + 1; 
        sleep(1);
    }

    /*for (int i = 0; i < MAX_KLIENCI; i++) {
        wait(NULL);  
    }*/
}

int main() {
    srand(time(NULL));

    key_t key = ftok(".", 'B');
if (key == -1) {
    perror("Błąd przy generowaniu klucza");
    exit(1);
}
key_t key_kk = ftok(".", 'C');
if (key_kk == -1) {
    perror("Błąd przy generowaniu klucza");
    exit(1);
}
key_t key_pamiec_kasjer = ftok(".", 'E');
if (key_pamiec_kasjer== -1) {
    perror("Błąd przy generowaniu klucza");
    exit(1);
}
key_t key_semafor_kasjer = ftok(".", 'F');
if (key_semafor_kasjer== -1) {
    perror("Błąd przy generowaniu klucza");
    exit(1);
}
int pamiec_kasjer = shmget(key_pamiec_kasjer, SHM_SIZE, IPC_CREAT | 0600);
    if (pamiec_kasjer == -1) {
        perror("Błąd przy tworzeniu pamięci współdzielonej");
        exit(1);
    }
int msgid = msgget(key_kk, IPC_CREAT | 0600);  
if (msgid == -1) {
    perror("Błąd przy tworzeniu kolejki");
    exit(1);
}
key_t key_kolejka_klienci = ftok(".", 'G');
    if (key_kolejka_klienci== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int kolejka_klienci = msgget(key_kolejka_klienci, IPC_CREAT | 0600);  
    if (kolejka_klienci == -1) {
        perror("Błąd przy tworzeniu kolejki");
        exit(1);
    }
    key_t key_kolejka_klienci2 = ftok(".", 'I');
    if (key_kolejka_klienci2== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int kolejka_klienci2 = msgget(key_kolejka_klienci2, IPC_CREAT | 0600);  
    if (kolejka_klienci2 == -1) {
        perror("Błąd przy tworzeniu kolejki");
        exit(1);
    }


int semafor_kasjer = utworz_semafor(key_semafor_kasjer, 1);
ustaw_semafor(semafor_kasjer, 0, 1);

generuj_klientow(key,msgid,pamiec_kasjer,semafor_kasjer,kolejka_klienci,kolejka_klienci2); 
return 0;
}   