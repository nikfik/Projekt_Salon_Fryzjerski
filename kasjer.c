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
volatile int emergency_closeup=200;
void zakocz_sie(int sig) {
    if (sig == SIGUSR2) {
        emergency_closeup=0;
    printf("Kasjer skonczyl dzialanie\n");
    exit(0);
    }
    else
    {
        printf("BLAD Sygnalu:\n");
    }
}

struct msgbuf {
    long mtype;  
    pid_t pid;
    int id;
};
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

int kasa[3]={0,0,0};//10,20,50
struct sembuf sb;
struct msgbuf2 {
    long mtype;  
    pid_t pid; 
    int liczba;
};
int obudz(int sig) {
    if (sig == SIGUSR1) {
       //printf("kasjer dostal sygnal");
    }
    else
    {
        printf("BLAD Sygnalu:v4\n");
    }
    return 0;
}

int suma(int *portfel)
{
    int sumuj=portfel[0] * 10 + portfel[1] * 20 + portfel[2] * 50;
    return sumuj;
}

int przelicz_pieniadze(int *banknoty)
{
    int tab[3]={10,20,50};
    int suma=0;
    for(int i=0;i<=2;i++)
    { 
        suma=suma+banknoty[i]*tab[i];
    }
    return suma;
}
void przelicz_reszte(int liczba, int *banknoty)
{
    int tab[3]={10,20,50};
    for (int i = 2; i>=0; i--)
    {
        if(liczba >=tab[i] )
        { 
        banknoty[i]++;
        kasa[i]--;
        liczba=liczba-tab[i];
        i++;
        }
    }
}

kasjer(int sem_kasjer,int pamiec_kasjer,int kolejka_klienci2)
{
    signal(SIGUSR1, obudz);
    signal(SIGUSR2, zakocz_sie);
    while(emergency_closeup){
    obsluz_klienta(sem_kasjer,pamiec_kasjer,kolejka_klienci2);     
    emergency_closeup--;
    }
};
    



void obsluz_klienta(int sem_kasjer,int pamiec_kasjer,int kolejka_klienci2)
{
    struct shared_memory *shared_mem = (struct shared_memory *)shmat(pamiec_kasjer, NULL, 0);
    shared_mem->pid_kasjera = getpid();
    struct msgbuf message;
    printf("kasjer czeka\n");
     if(msgrcv(kolejka_klienci2, &message, sizeof(struct msgbuf), 0, 0) == -1){
    printf("kasjer wpadl w sidla\n");
     sleep(1);
        exit(1);
    }
    else{
    printf("kasjer sie budzi\n");
    int cena = rand() % 4 + 5;
    cena=cena*10;
        shared_mem->kwota=cena;
        printf("kasjer podaje kwote do zaplaty: %d\n",cena);
        if(kill(shared_mem->pid_klienta, SIGUSR1)==-1){
        perror("Błąd przy wysyłaniu sygnału");
         }
        pause();//czekaj az zaplaci
        int zaplacone=przelicz_pieniadze(shared_mem->banknoty);  
        printf("kasjer dostal %dx10zl,%dx20zl,%dx50zl, w sumie %d\n",shared_mem->banknoty[0],shared_mem->banknoty[1],shared_mem->banknoty[2],zaplacone);
        przelicz_reszte(zaplacone-cena,shared_mem->reszta);
        printf("kasjer wydaje reszte:%dx10zl,%dx20zl,%dx50zl, w sumie %d\n",shared_mem->reszta[0],shared_mem->reszta[1],shared_mem->reszta[2],suma(shared_mem->reszta));

        if(kill(shared_mem->pid_klienta, SIGUSR1)==-1){
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
        };
        pause();

        //printf("SEMAFOR MA WARTOSC1 %d\n",semctl(sem_kasjer,0,GETVAL));
           //ustaw_semafor(sem_kasjer,0,2);
          // printf("SEMAFOR MA WARTOSC2 %d\n",semctl(sem_kasjer,0,GETVAL));
        //kill(shared_mem->pid_fryzjera,SIGUSR1);
        //printf("kasjer : wysyla sygnal do zniecierpliwionego fryzjera\n"); 
        shared_mem->zaplacone=0;   
        for (int i = 0;i < 3;i++) { 
        shared_mem->reszta[i]=0;         
        shared_mem->banknoty[i]=0;  
        }
    }
}




int main()
{
    srand(time(NULL));
    key_t key_pamiec_kasjer = ftok(".", 'E');
    if (key_pamiec_kasjer== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int pamiec_kasjer = shmget(key_pamiec_kasjer, SHM_SIZE, IPC_CREAT | 0600);
    if (pamiec_kasjer == -1) {
        perror("Błąd przy tworzeniu pamięci współdzielonej");
        exit(1);
    }

      key_t key_semafor_kasjer = ftok(".", 'F');
    if (key_semafor_kasjer== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    key_t key_kolejka_fryzjerzy = ftok(".", 'H');
    if (key_kolejka_fryzjerzy== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int kolejka_fryzjerzy = msgget(key_kolejka_fryzjerzy, IPC_CREAT | 0600);  
    if (kolejka_fryzjerzy == -1) {
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
    struct msgbuf message;
               message.mtype = 1;  
                 message.pid = getpid();
                 message.id=0;
                 if (msgsnd(kolejka_fryzjerzy, &message, sizeof(struct msgbuf), 0) == -1) 
                 {
                     perror("Błąd przy dodawaniu pida do kolejki fryzjerzy");
                     exit(1);
                 }
    int sem_kasjer = utworz_semafor(key_semafor_kasjer, 1);
    ustaw_semafor(sem_kasjer, 0, 1);
     struct shared_memory *shared_mem = (struct shared_memory *)shmat(pamiec_kasjer, NULL, 0);
    shared_mem->pid_kasjera = getpid();
    
    kasjer(sem_kasjer,pamiec_kasjer,kolejka_klienci2); 
return 0;
}