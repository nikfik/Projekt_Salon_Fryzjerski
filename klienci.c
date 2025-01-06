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

#define MAX_KLIENCI 20      
#define MAX_POCZEKALNIA 2 

struct sembuf sb;

int utworz_semafor(key_t key, int liczba_semaforow) {
    int semid = semget(key, liczba_semaforow, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("Błąd tworzenia semafora");
        exit(1);
    }
    return semid;
}

void ustaw_semafor(int semid, int num, int val) {
    if (semctl(semid, num, SETVAL, val) == -1) {
        perror("Błąd inicjalizacji semafora");
        exit(1);
    }
}


void klient(int id, int semid) {
    if(semctl(semid,0,GETVAL)>0)
    {
    sb.sem_num = 0;
    sb.sem_op = -1;  //P
    sb.sem_flg = 0;
    semop(semid, &sb, 1);  

    printf("Klient %d: Zajął miejsce w poczekalni.(%d/%d)\n", id,semctl(semid,0,GETVAL),MAX_POCZEKALNIA);

   
    sleep(3);//TUTAJ FUNKCJE CO ROBI KLIENT

    sb.sem_op = 1;  // V 
    semop(semid, &sb, 1);  

    printf("Klient %d: Opuszcza poczekalnię.\n", id);
    }
    else
    {
      printf("Nie było miejsca dla Klienta %d.\n", id);
    }
}


void generuj_klientow(key_t key) {
    int semid = utworz_semafor(key, 1);  // Tworzymy semafor poczekalni
    ustaw_semafor(semid, 0, MAX_POCZEKALNIA);  
    for (int i = 0; i < MAX_KLIENCI; i++) {
        pid_t pid = fork();  

        if (pid == 0) {  
            klient(i + 1, semid);
            exit(0);  
        } else if (pid < 0) {
            perror("Błąd przy tworzeniu procesu");
            exit(1);
        }

      
        int czas = rand() % 5 + 1;  // Losowy czas oczekiwania od 1 do 5 sekund
        sleep(czas);
    }

    // Czekamy, aż wszystkie procesy dzieci zakończą działanie
    for (int i = 0; i < MAX_KLIENCI; i++) {
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
    generuj_klientow(key);

    return 0;
}

