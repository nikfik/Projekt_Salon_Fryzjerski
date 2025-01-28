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
int main()
{
    key_t key_petla = ftok(".", 'D');
    if (key_petla == -1) {
        perror("Błąd przy generowaniu klucza");
        exit(1);
    }
    int sem_petla = utworz_semafor(key_petla, 1);
    int wybor=2;
    while(wybor)
    {
        scanf("%d",&wybor);
        printf("wybrano :%d\n",wybor);
        switch(wybor)
        {
            case 1:
            {
                cleanup();
                break;
            }
            case 2:
            {
                sb.sem_op = -1;  
                semop(sem_petla, &sb, 1);
                printf("\nopuszczam semafor \n");
                break;
            }
            case 3:
            {
                sb.sem_op = 1;  
                semop(sem_petla, &sb, 1);
                printf("\npodnosze semafor \n");
                break;
            }
        }
    }
    return 0;
}