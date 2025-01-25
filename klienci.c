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

#define MAX_KLIENCI 20      
#define MAX_POCZEKALNIA 7

struct sembuf sb;

int suma(int* portfel)
{
    int sumuj = portfel[0] * 10 + portfel[1] * 20 + portfel[2] * 50;
    return sumuj;
}



void obsluz_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Klient: Otrzymał sygnał, siada na fotelu.\n");
        sleep(5);
        printf("Klient: Zakończył strzyżenie i opuszcza salon.\n");
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
void zaplac(int* portfel, int cena)
{
    int nominaly[3] = { 10,20,50 };
    int zaplacone = 0;
    for (int i = 2; zaplacone < cena && i >= 0; i--)
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
        printf("Nie było miejsca dla Klienta %d.\n", id);
        return 0;
    }

}
void czekaj_na_fryzjera(int pid)
{

}

void zostan_ostrzyzonym()
{
    sleep(5);
}
void opusc_salon(int id, int semid)

{
    sb.sem_op = 1;
    semop(semid, &sb, 1);

    printf("Klient %d: Opuszcza poczekalnię.\n", id);
}

void klient(int id, int semid) {

    int* portfel = (int*)malloc(3 * sizeof(int));
    for (int i = 0;i < 3;i++) portfel[i] = 0;
    int cena = 80;//TEMP


    printf("Klient %d: Idzie zarabiać.\n", id);
    zarabiaj(portfel);
    printf("Klient %d: Ma teraz %dzł.\n", id, suma(portfel));
    if (zajmij_miejsce(id, semid))
    {
        czekaj_na_fryzjera(getpid());

        zostan_ostrzyzonym();
        zaplac(portfel, cena);
        //otrzymaj_reszte(*portfel)

        opusc_salon(id, semid);
    }

    signal(SIGUSR1, obsluz_signal);
    //pause();



}


void generuj_klientow(key_t key) {
    int semid = utworz_semafor(key, 1);
    ustaw_semafor(semid, 0, MAX_POCZEKALNIA);
    for (int i = 0; i < MAX_KLIENCI; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            klient(i + 1, semid);
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
    semctl(semid, 0, IPC_RMID);
}

int main() {
    srand(time(NULL));

    key_t key = ftok(".", 'B');
    if (key == -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    generuj_klientow(key);

    return 0;
}

