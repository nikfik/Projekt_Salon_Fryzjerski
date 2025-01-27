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
int main()
{
    printf("menu lol");
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
        }
    }
    return 0;
}