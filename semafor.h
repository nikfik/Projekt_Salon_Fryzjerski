#ifndef SEMAFOR_H
#define SEMAFOR_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int utworz_semafor(key_t key, int liczba_semaforow);
void ustaw_semafor(int semid, int num, int val);

#endif
