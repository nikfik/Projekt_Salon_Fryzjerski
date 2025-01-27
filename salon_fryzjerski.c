#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/msg.h>  
#include <sys/shm.h> 
#include "semafor.h"
struct komunikat {
    long mtype;
    pid_t klient_pid;
};

int main() {
    pid_t pid_fryzjerzy, pid_klienci,pid_debug;

    pid_fryzjerzy = fork();
    if (pid_fryzjerzy == 0) {
        printf("Uruchamiam proces fryzjerzy.c\n");
        execlp("./fryzjerzy", "./fryzjerzy", (char *)NULL); 
        perror("Błąd przy uruchamianiu fryzjerzy.c");
        exit(1);
    } else if (pid_fryzjerzy < 0) {
        perror("Błąd przy tworzeniu procesu fryzjerzy");
        exit(1);
    }
    pid_klienci = fork();
    if (pid_klienci == 0) {
        printf("Uruchamiam proces klient.c\n");
        execlp("./klienci", "./klienci", (char *)NULL);  
        perror("Błąd przy uruchamianiu klient.c");
        exit(1);
    } else if (pid_klienci < 0) {
        perror("Błąd przy tworzeniu procesu klient");
        exit(1);
    }
    pid_debug = fork();
    if (pid_debug == 0) {
        printf("Uruchamiam proces debug.c\n");
        execlp("./debug", "./debug", (char *)NULL);  
        perror("Błąd przy uruchamianiu debug.c");
        exit(1);
    } else if (pid_debug < 0) {
        perror("Błąd przy tworzeniu procesu debug");
        exit(1);
    }
    waitpid(pid_fryzjerzy, NULL, 0);
    waitpid(pid_klienci, NULL, 0);
    waitpid(pid_debug, NULL, 0);

  

    printf("Wszystkie procesy zostały zakończone.\n");
   
    return 0;
}
