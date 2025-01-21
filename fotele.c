#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include "semafor.h"

#define N 3  // Liczba foteli

struct sembuf sb;

// Funkcja zarządzająca fotelami
void zarzadzaj_fotela(int semid, int fotel_id) {
    sb.sem_num = fotel_id;  // Semafor dla fotela
    sb.sem_op = -1;  // P (czekanie na dostępność fotela)
    sb.sem_flg = 0;
    semop(semid, &sb, 1);  // Oczekiwanie na dostępność fotela

    printf("Klient usiadł na fotelu %d.\n", fotel_id);

    sleep(5);  // Czas strzyżenia (symulacja)

    sb.sem_op = 1;  // V (zwolnienie fotela)
    semop(semid, &sb, 1);

    printf("Klient opuszcza fotel %d.\n", fotel_id);
}

int main() {
    key_t key = ftok(".", 'A');  
    if (key == -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }

    int semid = utworz_semafor(key, N); 
    for (int i = 0; i < N; i++) {
        ustaw_semafor(semid, i, 1);  
    }

    for (int i = 0; i < N; i++) {
        zarzadzaj_fotela(semid, i);  
    }

    semctl(semid, 0, IPC_RMID);  

    return 0;
}
