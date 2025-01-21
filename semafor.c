#include "semafor.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>

int utworz_semafor(key_t key, int liczba_semaforow) {
    int semid = semget(key, liczba_semaforow, IPC_CREAT | 0600);
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
