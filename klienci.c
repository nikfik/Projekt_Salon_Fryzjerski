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

#define MAX_KLIENCI 1      
#define MAX_POCZEKALNIA 2
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
};
struct msgbuf2 {
    long mtype;  
    pid_t pid; 
    int liczba;
};
struct sembuf sb;

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

void obsluz_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("klient: otrzymal sygnal, idzie do fotela.\n");
    }
    else
    {
        printf("BLAD Sygnalu:v1\n");
    }
}
void obsluz_signal2(int sig, siginfo_t *info,void *ucontext) {
    if (sig == SIGUSR2) {
        printf("klient: Otrzymał inta %d\n",info->si_value.sival_int);
        kill(info->si_value.sival_int, SIGUSR1) == -1;//synchronizacja  
    }
    else
    {
        printf("BLAD Sygnalu:v2\n");
    }
}
void obudz(int sig) {
    if (sig == SIGUSR1) {
    }
    else
    {
        printf("BLAD Sygnalu:v3\n");
    }
}
void zarabiaj(int* portfel)
{
    int liczba = rand() % 3 + 1;
   // sleep(1);
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

        printf("klient %d: Zajął miejsce w poczekalni.(%d/%d)\n", id, semctl(semid, 0, GETVAL), MAX_POCZEKALNIA);
        return 1;
    }
    else
    {
        printf("klient %d: Nie było miejsca dla klienta wiec wychodzi.\n", id);
        return 0;
    }

}

void czekaj_na_fryzjera(int id,int pid,int msgid)
{
struct msgbuf message;
    message.mtype = 1;  
    message.pid = pid;

if (msgsnd(msgid, &message, sizeof(pid_t), 0) == -1) 
    {
        perror("Błąd przy wysyłaniu wiadomości");
        exit(1);
    }

      printf("klient %d: wyslal swoj pid[%d] do kolejki\n",id,pid);
    signal(SIGUSR1, SIG_IGN);  
}


void usiadz_na_fotelu(int id)
{ 
    signal(SIGUSR1, obsluz_signal);
    
    pause();
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = obsluz_signal2;  
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd przy rejestracji sygnału");
        exit(1);
    }
    printf("klient %d usiadl na fotelu\n",id);
}



void zaplac(int id,int *portfel,int pamiec_kasjer,int semafor_kasjer)
{
    printf("klient %d chce zaplacic\n",id);
    signal(SIGUSR1, obudz);
    pause();
        sb.sem_num = 0;
        sb.sem_op = -1; 
        sb.sem_flg = 0;
        semop(semafor_kasjer, &sb, 1);
        
        struct shared_memory *shared_mem = (struct shared_memory *)shmat(pamiec_kasjer, NULL, 0);
        if(shared_mem == (void *)-1) {
        perror("Błąd przy przypisaniu pamięci współdzielonej");
        exit(1);
        }
        shared_mem->pid_klienta = getpid();

        printf("klient %d: budzi kasjera\n",id);
        if(kill(shared_mem->pid_kasjera, SIGUSR1) == -1){//obudz kasjera
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);}
        pause();
    
        int nominaly[3] = { 10,20,50 };
        int zaplacone = 0;
        printf("klient %d:szuka pieniedzy w portfelu by zaplacic %d\n",id,shared_mem->kwota);
        printf("klient %d ma: %dx10zl, %dx20zl, %dx50zl\n",id,portfel[0],portfel[1],portfel[2]);
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
    printf("klient %d:placi%d z %d\n",id,zaplacone,shared_mem->kwota);
    if(kill(shared_mem->pid_kasjera, SIGUSR1) == -1){//zaplac
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    }
    pause();

    printf("klient %d: dostaje reszte\n",id);
    for(int i=0;i<=2;i++)
    { 
        portfel[i]+=shared_mem->reszta[i];
    }
   // printf("pid fryzjera:%d\n",shared_mem->pid_fryzjera);
    printf("klient %d: po wydaniu reszty ma %dx10zl, %dx20zl, %dx50zl\n",id,portfel[0],portfel[1],portfel[2]);
    if(kill(shared_mem->pid_fryzjera, SIGUSR1) == -1){
        perror("Błąd przy wysyłaniu sygnału1");
        exit(1);
    };
     if(kill(shared_mem->pid_kasjera, SIGUSR1) == -1){
        perror("Błąd przy wysyłaniu sygnału2");
        exit(1);
    };
}





void zostan_ostrzyzonym(int id)
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = obsluz_signal2;  
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("Błąd przy rejestracji sygnału");
        exit(1);
    } 
    pause();
    printf("Klient %d: schodzi z fotela zadowolony\n",id);
}

void opusc_salon(int id, int semid)
{
    sb.sem_op = 1;  
    semop(semid, &sb, 1);

    printf("Klient %d: Opuszcza poczekalnię.(%d/%d)\n", id, semctl(semid, 0, GETVAL), MAX_POCZEKALNIA);
}

void klient(int id, int semid,int msgid,int sem_petla,int pamiec_kasjer,int semafor_kasjer) {
 int emergency_closeup=1000000;
 int *portfel =(int*) malloc(3*sizeof(int));
    for (int i = 0;i < 3;i++) portfel[i] = 0;
 while(emergency_closeup){while(semctl(sem_petla,0,GETVAL)){
        
        int temp_money=suma(portfel);
        printf("Klient %d: Idzie zarabiać.\n",id);
        zarabiaj(portfel);
        printf("Klient %d: zarobił %dzł.[%d]->[%d]\n",id,suma(portfel)-temp_money,temp_money,suma(portfel));
        sleep(1);
        if(zajmij_miejsce(id, semid))
        {
        czekaj_na_fryzjera(id,getpid(),msgid);
        usiadz_na_fotelu(id);
        temp_money=suma(portfel);
        zaplac(id,portfel,pamiec_kasjer,semafor_kasjer);
        printf("Klient %d: zaplacil .[%d]->[%d]\n",id,temp_money,suma(portfel));
        zostan_ostrzyzonym(id);
        
        opusc_salon(id, semid);
        
        }

    }}
  
}


void generuj_klientow(key_t key,int msgid,int sem_petla,int pamiec_kasjer,int semafor_kasjer) {
    int semid = utworz_semafor(key, 1);  
    ustaw_semafor(semid, 0, MAX_POCZEKALNIA);
    for (int i = 0; i < MAX_KLIENCI; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            klient(i + 1, semid,msgid,sem_petla,pamiec_kasjer,semafor_kasjer);
            exit(0);
        }
        else if (pid < 0) {
            perror("Błąd przy tworzeniu procesu");
            exit(1);
        }


        int czas = rand() % 5 + 1; 
        sleep(czas);
    }

    for (int i = 0; i < MAX_KLIENCI; i++) {
        wait(NULL);  
    }
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
key_t key_petla = ftok(".", 'D');
if (key_petla == -1) {
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
int sem_petla = utworz_semafor(key_petla, 1);
int semafor_kasjer = utworz_semafor(key_semafor_kasjer, 1);
ustaw_semafor(sem_petla, 0, 1);
ustaw_semafor(semafor_kasjer, 0, 1);

generuj_klientow(key,msgid,sem_petla,pamiec_kasjer,semafor_kasjer); 
return 0;
}