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
int utworz_pamiec_wspolna(key_t key, int rozmiar) {
    int shmid = shmget(key, rozmiar, IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("Błąd przy tworzeniu pamięci współdzielonej");
        exit(1);
    }
    return shmid;
}
void* przypisz_pamiec(int shmid) {
    void *shmptr = shmat(shmid, NULL, 0);
    if (shmptr == (void*) -1) {
        perror("Błąd przy przypisaniu pamięci współdzielonej");
        exit(1);
    }
    return shmptr;
}
void cleanup()
{
     key_t keyA = ftok(".", 'A');
    if (keyA == -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    key_t keyB = ftok(".", 'B');
    if (keyB == -1) {
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
    int msgid = msgget(key_kk, IPC_CREAT | 0600);  
    if (msgid == -1) {
        perror("Błąd przy tworzeniu kolejki");
        exit(1);
    }
    key_t key_pamiec_kasjer = ftok(".", 'E');
    if (key_pamiec_kasjer== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int pamiec_kasjer = shmget(key_pamiec_kasjer, 0, IPC_CREAT | 0600);
    if (pamiec_kasjer == -1) {
        perror("Błąd przy tworzeniu pamięci współdzielonej");
        exit(1);
    }
      key_t key_semafor_kasjer = ftok(".", 'F');
    if (key_semafor_kasjer== -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int semidA = utworz_semafor(keyA, 1);
    int semidB = utworz_semafor(keyB, 1);
    int semidD = utworz_semafor(key_petla, 1);
    int semidF = utworz_semafor(key_semafor_kasjer, 1);
    shmctl(pamiec_kasjer, IPC_RMID, NULL);//pamiec kasjer
    msgctl(msgid, IPC_RMID, NULL);//kolejka komunikatow
    semctl(semidA, 0, IPC_RMID);//semafor foteli
    semctl(semidB, 0, IPC_RMID);//semafor poczekalni
    semctl(semidD, 0, IPC_RMID);//semafor petli
    semctl(semidF, 0, IPC_RMID);//semafor kasjer
    printf("Wszystko posprzatane.\n");
}