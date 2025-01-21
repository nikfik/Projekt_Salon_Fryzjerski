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

#define MAX_FRYZJER 3      

struct sembuf sb;


void wez_fotel(int fryzjer_pid) {
    printf("Fryzjer: Wysyłam sygnał do klienta, aby usiadł na fotelu.\n");
    kill(fryzjer_pid, SIGUSR1);  // Wysyłamy sygnał fryzjerowi
    kill(fryzjer_pid, SIGUSR2);
}


void fryzjer(int id, int semid) {
    if(semctl(semid,0,GETVAL)>0)
    {
    sb.sem_num = 0;
    sb.sem_op = -1;  //P
    sb.sem_flg = 0;
    semop(semid, &sb, 1);  

    printf("fryzjer %d: w gotowości.(%d/%d)\n", id,semctl(semid,0,GETVAL),MAX_FRYZJER);

   
    sleep(15);//TUTAJ FUNKCJE CO ROBI FRYZJER
     wez_fotel(getpid());

    sb.sem_op = 1;  // V 
    semop(semid, &sb, 1);  

    printf("fryzjer %d: udaje sie na przerwę.\n", id);
    }
    else
    {
      printf("osiągnięto limit fryzjerów %d.\n", id);
    }
}


void generuj_fryzjerow(key_t key) {
    int semid = utworz_semafor(key, 1); 
    ustaw_semafor(semid, 0, MAX_FRYZJER);  
    for (int i = 0; i < MAX_FRYZJER; i++) {
        pid_t pid = fork();  

        if (pid == 0) {  
            fryzjer(i + 1, semid);
            exit(0);  
        } else if (pid < 0) {
            perror("Błąd przy tworzeniu procesu");
            exit(1);
        }
    }

    // Czekamy, aż wszystkie procesy dzieci zakończą działanie
    for (int i = 0; i < MAX_FRYZJER; i++) {
        wait(NULL);  // Czekamy na zakończenie każdego procesu klienta
    }

    // Usuwamy semafory po zakończeniu wszystkich procesów
    semctl(semid, 0, IPC_RMID);
}

int main() {
    srand(time(NULL)); 

    key_t key = ftok(".", 'A');  // Tworzymy unikalny klucz dla semafora
    if (key == -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    generuj_fryzjerow(key);

    return 0;
}

