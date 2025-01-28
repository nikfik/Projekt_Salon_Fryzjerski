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
struct sembuf sb;
struct msgbuf {
    long mtype;  
    pid_t pid; 
    int id;
};
struct shared_data {
    int tp;
    int tk;
};
void zegar(struct shared_data *shared) {
    shared->tp=8;
    shared->tk=11;
    while (shared->tp < shared->tk) { 
        printf("--------godzina %d:00----------\n",shared->tp);
        sleep(15); 
        shared->tp++;
    }
    printf("ZAMYKAMY SALON\n");
}




void kill_fryzjer(pid_t pid) {
    if (kill(pid, SIGUSR2) == -1) {
        usleep(100000);
        //perror("Błąd przy wysyłaniu sygnału do fryzjera");
        //exit(1);
    }
    //printf("Wysłano sygnał do fryzjera %d, aby opuścił salon.\n", pid);
}

void kill_klient(pid_t pid) {
    //printf("proba zabicia klienta %d\n",pid);
    if (kill(pid, SIGUSR2) == -1) {
        usleep(100000);
        //perror("Błąd przy wysyłaniu sygnału do klienta");
        //exit(1);
    }
    //printf("Wysłano sygnał do klienta %d, aby opuścił salon.\n", pid);
}

void kill_all(int kolejka_klienci, int kolejka_fryzjerzy) {
    struct msgbuf message;
    pid_t klient_pid, fryzjer_pid;

   while (msgrcv(kolejka_klienci, &message, sizeof(struct msgbuf), 0, IPC_NOWAIT) != -1) {
    klient_pid = message.pid;
    printf("konczymy klienta %d\n",klient_pid);
    kill_klient(klient_pid);
}
while (msgrcv(kolejka_fryzjerzy, &message, sizeof(struct msgbuf), 0, IPC_NOWAIT) != -1) {
    fryzjer_pid = message.pid;
    printf("konczymy fryzjera %d\n",fryzjer_pid);
    kill_fryzjer(fryzjer_pid);
}


    printf("Wszystkie procesy klientów i fryzjerów zostały zakończone.\n");
}



int main()
{

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
    key_t key_pamiec_zegar = ftok(".", 'D');
    if (key_pamiec_zegar == -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int shm_id = shmget(key_pamiec_zegar, sizeof(struct shared_data), IPC_CREAT | 0600);
    if (shm_id == -1) {
        perror("Błąd przy tworzeniu pamięci dzielonej");
        exit(1);
    }
    struct shared_data *shared = shmat(shm_id, NULL, 0);
    if (shared == (void *) -1) {
        perror("Błąd przy podłączaniu pamięci dzielonej");
        exit(1);
    }

    pid_t pid_zegar = fork();
    if (pid_zegar == 0) {
        zegar(shared);
        kill_all(kolejka_fryzjerzy,kolejka_klienci);
        cleanup();
        shmdt(shared);
    } 
    else
    {
        if (pid_zegar < 0) {
        perror("Błąd przy uruchamianiu zegara");
        exit(1);
         }
    int wybor=2;
    while(wybor&&shared->tp < shared->tk)
    {
        scanf("%d",&wybor);
        printf("wybrano :%d\n",wybor);
        switch(wybor)
        {
            case 0:
            {
                shared->tp = shared->tk;
                break;
            }
            case 1:
            {
                cleanup();
                break;
            }
        }
    }
    }
    shmdt(shared);
    return 0;
}