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


#define wielkosc_poczekalni 10;

int sem_poczekalnia;

void utworz_nowy_semafor(void)
  {
    sem_poczekalnia=semget(110370,1,0666|IPC_CREAT);
    if (sem_poczekalnia==-1)
      {
        printf("Nie moglem utworzyc nowego semafora.\n");
        exit(EXIT_FAILURE);
      }
    else
      {
	printf("Semafor zostal utworzony : %d\n",sem_poczekalnia);
      }
  }
void semafor_p(int zmien_sem)
  {
   
    struct sembuf bufor_sem;
    bufor_sem.sem_num=0;
    bufor_sem.sem_op=-1;
    bufor_sem.sem_flg=0;
    zmien_sem=semop(sem_poczekalnia,&bufor_sem,1);
    if (zmien_sem==-1) 
      {
	if(errno == EINTR){
	semafor_p(zmien_sem);
	}
	else
	{
        printf("Nie moglem zamknac semafora.\n");
        exit(EXIT_FAILURE);
	}
      }
    else
      {
        printf("Semafor zostal zamkniety.\n");
      }
  }

void semafor_v(int zmien_sem)
  {
    struct sembuf bufor_sem;
    bufor_sem.sem_num=0;
    bufor_sem.sem_op=1;
    bufor_sem.sem_flg=SEM_UNDO;
    zmien_sem=semop(sem_poczekalnia,&bufor_sem,1);
    if (zmien_sem==-1) 
      {
        printf("Nie moglem otworzyc semafora.\n");
        exit(EXIT_FAILURE);
      }
    else
      {
        printf("Semafor zostal otwarty.\n");
      }
  }

static void usun_semafor(void)  
  {
    int sem;
    sem=semctl(sem_poczekalnia,0,IPC_RMID);
    if (sem==-1)
      {
        printf("Nie mozna usunac semafora.\n");
        exit(EXIT_FAILURE);
      }
    else
      {
	printf("Semafor zostal usuniety : %d\n",sem);
      }
  }




int main()
{
utworz_nowy_semafor();
printf("%d,%d",sem_poczekalnia,semctl(sem_poczekalnia,0,GETVAL));

return 0;
}