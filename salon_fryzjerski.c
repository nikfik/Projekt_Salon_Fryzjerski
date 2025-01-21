#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    pid_t pid_fryzjerzy, pid_fotele, pid_klienci;

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
    pid_fotele = fork();
    if (pid_fotele == 0) {
        printf("Uruchamiam proces fotele.c\n");
        execlp("./fotele", "./fotele", (char *)NULL);  
        perror("Błąd przy uruchamianiu fotele.c");
        exit(1);
    } else if (pid_fotele < 0) {
        perror("Błąd przy tworzeniu procesu fotele");
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
    waitpid(pid_fryzjerzy, NULL, 0);
    waitpid(pid_fotele, NULL, 0);
    waitpid(pid_klienci, NULL, 0);

    printf("Wszystkie procesy zostały zakończone.\n");
    return 0;
}
