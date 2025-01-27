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

struct msgbuf {
    long mtype;  
    pid_t pid;   
};

void znajdz_fotel(int id,int semid,pid_t mpid) {
if(semctl(semid,0,GETVAL)>0)
{
sb.sem_num = 0;
sb.sem_op = -1;  //P
sb.sem_flg = 0;
semop(semid, &sb, 1); 
 if (kill(mpid, SIGUSR1) == -1) {
        perror("Błąd przy wysyłaniu sygnału");
        exit(1);
    }
else{
    printf("fryzjer %d poprosil klienta %d na fotel %d\n",id,mpid,MAX_FOTEL-semctl(semid,0,GETVAL));
    }
}

}


void fryzjer(int id, int msmid,key_t key_kk,int semid) {
    

    printf("fryzjer %d: w gotowości.\n", id);

    struct msgbuf message;
    int msgid = msgget(key_kk, 0600);  
    if (msgid == -1) {
        perror("Błąd przy uzyskiwaniu dostępu do kolejki");
        exit(1);
    }
    if(msgrcv(msgid, &message, sizeof(pid_t), 0, 0) == -1)
    {
        //printf("fryzjer %d:",id);   
        perror("Błąd przy odbieraniu wiadomości");
        exit(1);
    }
    else
    {
        printf("fryzjer %d Otrzymanł PID: %d z kolejki komunikatów\n",id, message.pid);
        znajdz_fotel(id,semid,message.pid);
        //powiedz_cene();
        //ostrzyz();
        //wylicz();
        sleep(5);
        sb.sem_op = 1;  // V 
        semop(semid, &sb, 1);
    }

    

    printf("fryzjer %d: udaje sie na przerwę.\n", id);
    
   
}


void generuj_fryzjerow(key_t key, int msgid,key_t key_kk) {
    int semid = utworz_semafor(key, 1); 
    ustaw_semafor(semid, 0, MAX_FOTEL);  
    for (int i = 0; i < MAX_FRYZJER; i++) {
        pid_t pid = fork();  

        if (pid == 0) {  
            fryzjer(i + 1, msgid,key_kk,semid);
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
    int msgid = msgget(key_kk, IPC_CREAT | 0600);  
    if (msgid == -1) {
        perror("Błąd przy tworzeniu kolejki");
        exit(1);
    }
    generuj_fryzjerow(key,msgid,key_kk);

    return 0;
}
