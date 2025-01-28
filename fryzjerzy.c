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
#include <stdarg.h>
#define MAX_FRYZJER 4    
#define MAX_FOTEL 2

void zapisz(const char *format, ...) {
    FILE *file = fopen("fryzjer_summary.txt", "a");
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
    int id;
};
void zakocz_sie(int sig) {
    if (sig == SIGUSR2) {
    zapisz("fryzjer skonczyl dzialanie\n");
    exit(0);
    }
    else
    {
        zapisz("BLAD Sygnalu:\n");
    }
}
void obsluz_signal_odblokuj(int sig) {
    if (sig == SIGUSR1) {
        //sleep(1);
    }
    else
    {
        printf("BLAd Sygnalu\n");
    }
}



void znajdz_fotel(int id,int semid,int idk,pid_t mpid) {
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
    zapisz("fryzjer %d poprosil klienta %d na fotel %d\n",id,idk,MAX_FOTEL-semctl(semid,0,GETVAL));
    sleep(1);
    union sigval value;
    value.sival_int = getpid();
      if (sigqueue(mpid, SIGUSR1, value) == -1) {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
      }
      zapisz("fryzjer %d [%d] wyslal do %d \n",id,getpid(),mpid);
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
    zapisz("fryzjer %d: czeka az klient zaplaci\n",id);
    signal(SIGUSR1, obsluz_signal_odblokuj);
    while(semctl(semafor_kasjer,0,GETVAL)!=2){printf("",semctl(semafor_kasjer,0,GETVAL));}
    semop(semafor_kasjer, &sb, 1); 
    zapisz("fryzjer %d: wysyła sygnał do klienta\n",id);
    if (kill(mpid, SIGUSR1) == -1) 
    {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    } 
    pause();//czekaj az klient skonczy placic
}

void ostrzyz(int semid,int id,int pid_klienta)
{
    zapisz("fryzjer %d strzyze...\n",id);
    sleep(5);
    signal(SIGUSR1, obsluz_signal_odblokuj);
    zapisz("fryzjer %d ostrzygl\n",id);
    union sigval value;
    value.sival_int = getpid();
    //zapisz("proba wyslania sygnalu do%d\n",pid_klienta);
      if (sigqueue(pid_klienta, SIGUSR1, value) == -1) {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    }
    pause();
    sb.sem_op = 1;
    semop(semid, &sb, 1);
    //printf("semafor fotela1=%d",semctl(semid,0,GETVAL));
}
void idz_na_przerwe_chyba(int id)
{
    int czy_zasluzyl = rand()%4;
    if(czy_zasluzyl==3) 
    {
        zapisz("fryzjer %d: udaje sie na przerwę.\n", id);
        sleep(10);
        zapisz("fryzjer %d: wraca z przerwy.\n", id);
    }
}





void fryzjer(int id, int msgid,int semid,int semafor_kasjer,int pamiec_kasjer) {
    signal(SIGUSR2, zakocz_sie);
    int emergency_closeup=20;

    zapisz("fryzjer %d: w gotowości[%d].\n", id,getpid());

    struct msgbuf message;
    while(emergency_closeup>=0)
    {
    if(msgrcv(msgid, &message, sizeof(struct msgbuf), 0, 0) == -1)
    {
        //printf("fryzjer %d: blad :%d,%d,%d\n",id,message.id,message.mtype,message.pid);
       // perror("Błąd przy odbieraniu wiadomości");
       sleep(1);
        exit(1);
    }
    else
    {
        zapisz("fryzjer %d Otrzymanł PID: %d klienta %d z kolejki komunikatów\n",id, message.pid,message.id);
        znajdz_fotel(id,semid,message.id,message.pid);
        czekaj_na_klienta(id,semafor_kasjer,message.pid,pamiec_kasjer);
        ostrzyz(semid,id,message.pid);
        idz_na_przerwe_chyba(id);
    }
    emergency_closeup--;
    }
    

   
}


void generuj_fryzjerow(key_t key, int msgid,int semafor_kasjer,int pamiec_kasjer,int kolejka_fryzjerzy) {
    int semid = utworz_semafor(key, 1); 
    ustaw_semafor(semid, 0, MAX_FOTEL);  
    for (int i = 0; i < MAX_FRYZJER; i++) {
        pid_t pid = fork();  
        if (pid == 0) {  
            struct msgbuf message;
               message.mtype = 1;  
                 message.pid = getpid();
                 message.id=i+1;
                 if (msgsnd(kolejka_fryzjerzy, &message, sizeof(struct msgbuf), 0) == -1) 
                 {
                     perror("Błąd przy dodawaniu pida do kolejki fryzjerzy");
                     exit(1);
                 }
            fryzjer(i + 1, msgid,semid,semafor_kasjer,pamiec_kasjer);
            exit(0);  
        } else if (pid < 0) {
            perror("Błąd przy tworzeniu procesu fryzjera");
            exit(1);
        }
    }


    /*for (int i = 0; i < MAX_FRYZJER; i++) {
        wait(NULL);  
    }*/
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

    int sem_kasjer = utworz_semafor(key_semafor_kasjer, 1);
    generuj_fryzjerow(key,msgid,sem_kasjer,pamiec_kasjer,kolejka_fryzjerzy);

    return 0;
}
