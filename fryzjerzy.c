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
#define MAX_FRYZJER 5    
#define MAX_FOTEL 3

struct sembuf sb;
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

void obsluz_signal_odblokuj(int sig) {
    if (sig == SIGUSR1) {
        sleep(1);
    }
    else
    {
        printf("BLAd Sygnalu\n");
    }
}



void znajdz_fotel(int id,int semid,pid_t mpid) {
if(semctl(semid,0,GETVAL)>0)
{
sb.sem_num = 0;
sb.sem_op = -1;  
sb.sem_flg = 0;
semop(semid, &sb, 1); 
 if (kill(mpid, SIGUSR1) == -1) {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    }
else{
    printf("fryzjer %d poprosil klienta %d na fotel %d\n",id,mpid,MAX_FOTEL-semctl(semid,0,GETVAL));
    sleep(1);
    union sigval value;
    value.sival_int = getpid();
      if (sigqueue(mpid, SIGUSR2, value) == -1) {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
      }
      printf("fryzjer %d [%d] wyslal do %d \n",id,getpid(),mpid);
    }
}   

}

void czekaj_na_klienta(int id,int semafor_kasjer,pid_t mpid,int pamiec_kasjer)
{
     struct shared_memory *shared_mem = (struct shared_memory *)shmat(pamiec_kasjer, NULL, 0);
    shared_mem->pid_fryzjera=getpid();
    sb.sem_num = 0;
    sb.sem_op = -1;  
    sb.sem_flg = 0;
    printf("fryzjer %d: czeka az klient zaplaci\n",id);
    signal(SIGUSR1, obsluz_signal_odblokuj);
    while(semctl(semafor_kasjer,0,GETVAL)!=2){/*printf("\nspam1:%d\n",semctl(semafor_kasjer,0,GETVAL));*/}
    semop(semafor_kasjer, &sb, 1); 
    printf("fryzjer %d: wysyła sygnał do klienta\n",id);
    if (kill(mpid, SIGUSR1) == -1) 
    {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    } 
    pause();//czekaj az klient skonczy placic
}

void ostrzyz(int semid,int id,int sepid)
{
    printf("fryzjer %d strzyze...\n",id);
    sleep(5);
    signal(SIGUSR1, obsluz_signal_odblokuj);
    union sigval value;
    value.sival_int = getpid();
      if (sigqueue(sepid, SIGUSR2, value) == -1) {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    }
    pause();
    printf("fryzjer %d ostrzygl\n",id);
    sb.sem_op = 1;
    semop(semid, &sb, 1);
    //printf("semafor fotela1=%d",semctl(semid,0,GETVAL));
}
void idz_na_przerwe_chyba(int id)
{
    int czy_zasluzyl = rand()%4;
    if(czy_zasluzyl==3) 
    {
        printf("fryzjer %d: udaje sie na przerwę.\n", id);
        sleep(10);
        printf("fryzjer %d: wraca z przerwy.\n", id);
    }
}
void fryzjer(int id, int msmid,key_t key_kk,int semid,int sem_petla,int semafor_kasjer,int pamiec_kasjer) {
    int emergency_closeup=1000000;

    printf("fryzjer %d: w gotowości[%d].\n", id,getpid());

    struct msgbuf message;
    int msgid = msgget(key_kk, 0600);  
    if (msgid == -1) {
        perror("Błąd przy uzyskiwaniu dostępu do kolejki");
        exit(1);
    }
    while(emergency_closeup){while(semctl(sem_petla,0,GETVAL))
    {
    if(msgrcv(msgid, &message, sizeof(pid_t), 0, 0) == -1)
    {
        perror("Błąd przy odbieraniu wiadomości");
        exit(1);
    }
    else
    {
        printf("fryzjer %d Otrzymanł PID: %d z kolejki komunikatów\n",id, message.pid);
        znajdz_fotel(id,semid,message.pid);
        czekaj_na_klienta(id,semafor_kasjer,message.pid,pamiec_kasjer);
        ostrzyz(semid,id,semid);
        idz_na_przerwe_chyba(id);
    }
    }
    emergency_closeup--;
    }
    

   
}


void generuj_fryzjerow(key_t key, int msgid,key_t key_kk,int sem_petla,int semafor_kasjer,int pamiec_kasjer) {
    int semid = utworz_semafor(key, 1); 
    ustaw_semafor(semid, 0, MAX_FOTEL);  
    for (int i = 0; i < MAX_FRYZJER; i++) {
        pid_t pid = fork();  

        if (pid == 0) {  
            fryzjer(i + 1, msgid,key_kk,semid,sem_petla,semafor_kasjer,pamiec_kasjer);
            exit(0);  
        } else if (pid < 0) {
            perror("Błąd przy tworzeniu procesu fryzjera");
            exit(1);
        }
    }


    for (int i = 0; i < MAX_FRYZJER; i++) {
        wait(NULL);  
    }
}

int main() {
    srand(time(NULL)); 

    key_t key = ftok(".", 'A');  
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
    int msgid = msgget(key_kk, IPC_CREAT | 0600);  
    if (msgid == -1) {
        perror("Błąd przy tworzeniu kolejki");
        exit(1);
    }
    int sem_petla = utworz_semafor(key_petla, 1);
    ustaw_semafor(sem_petla, 0, 1); 
    int sem_kasjer = utworz_semafor(key_semafor_kasjer, 1);
    generuj_fryzjerow(key,msgid,key_kk,sem_petla,sem_kasjer,pamiec_kasjer);

    return 0;
}
