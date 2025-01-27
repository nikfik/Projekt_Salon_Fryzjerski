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

#define MAX_KLIENCI 3      
#define MAX_POCZEKALNIA 2
struct msgbuf {
    long mtype;  
    pid_t pid;  
};

struct sembuf sb;

int suma(int *portfel)
{
    int sumuj=portfel[0] * 10 + portfel[1] * 20 + portfel[2] * 50;
    return sumuj;
}



void obsluz_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Klient: Otrzymał sygnał, siada na fotelu.\n");
        sleep(5);
    }
    else
    {
        printf("BLAD Sygnalu\n");
    }
}


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
void zaplac(int id,int *portfel,int cena)
{
    printf("klient %d: zaplacil\n",id);
    int nominaly[3] = { 10,20,50 };
    int zaplacone = 0;
    for (int i = 2; zaplacone < cena && i>=0; i--)
    {
        portfel[i]--;
        zaplacone = zaplacone + nominaly[i];
    }
}


int zajmij_miejsce(int id, int semid)
{
    if (semctl(semid, 0, GETVAL) > 0)
    {
        sb.sem_num = 0;
        sb.sem_op = -1; 
        sb.sem_flg = 0;
        semop(semid, &sb, 1);

        printf("Klient %d: Zajął miejsce w poczekalni.(%d/%d)\n", id, semctl(semid, 0, GETVAL), MAX_POCZEKALNIA);
        return 1;
    }
    else
    {
        printf("Klient %d: Nie było miejsca dla Klienta wiec wychodzi.\n", id);
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

      printf("Klient %d: wyslal swoj pid[%d] do kolejki\n",id,pid);
    signal(SIGUSR1, SIG_IGN);  
}
void zostan_ostrzyzonym(int id)
{
    signal(SIGUSR1, obsluz_signal);
    sleep(5);
    printf("Klient %d: Zakończył strzyżenie\n",id);
    sleep(1);
    }

void opusc_salon(int id, int semid)
{
    sb.sem_op = 1;  
    semop(semid, &sb, 1);

    printf("Klient %d: Opuszcza poczekalnię.(%d/%d)\n", id, semctl(semid, 0, GETVAL), MAX_POCZEKALNIA);
}

void klient(int id, int semid,int msgid) {
 
    int *portfel =(int*) malloc(3*sizeof(int));
    for (int i = 0;i < 3;i++) portfel[i] = 0;
    int cena =80;//TEMP

        
        printf("Klient %d: Idzie zarabiać.\n",id);
        zarabiaj(portfel);
        printf("Klient %d: Ma teraz %dzł.\n",id,suma(portfel));
        sleep(1);
        if(zajmij_miejsce(id, semid))
        {
        czekaj_na_fryzjera(id,getpid(),msgid);
        zaplac(id,portfel,cena);
        zostan_ostrzyzonym(id);

        //otrzymaj_reszte(*portfel)
        
        opusc_salon(id, semid);
        
        }


  
}


void generuj_klientow(key_t key,int msgid) {
    int semid = utworz_semafor(key, 1);  
    ustaw_semafor(semid, 0, MAX_POCZEKALNIA);
    for (int i = 0; i < MAX_KLIENCI; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            klient(i + 1, semid,msgid);
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
    int msgid = msgget(key_kk, IPC_CREAT | 0600);  
    if (msgid == -1) {
        perror("Błąd przy tworzeniu kolejki");
        exit(1);
    }
    generuj_klientow(key,msgid); 
    return 0;
}

