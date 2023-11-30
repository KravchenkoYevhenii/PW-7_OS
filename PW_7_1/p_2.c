//
// Created by vboxuser on 11/30/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>

int shm_id;
void *shm_ptr;

void signal_handler(int sig) {
    printf("\nChild program | signal handler called with signal: %d\n", sig);
    if (sig == SIGUSR1) {
        int sum = 0, i = 0, val;
        printf("Child program | Reading data from shared memory...\n");

        do {
            memcpy(&val, shm_ptr + i * sizeof(int), sizeof(int));
            if (val == 0) break;
            sum += val;
            i++;
        } while (1);

        printf("Child program | Sum: %d\n", sum);

        memcpy(shm_ptr, &sum, sizeof(int));

        kill(getppid(), SIGUSR1);
        printf("Child program |  Signal sent back\n");
    }
}

void attach_shared_memory(char *shm_id_str) {
    shm_id = atoi(shm_id_str);
    printf("\nChild program | Shared memory ID received: %d\n", shm_id);
    shm_ptr = shmat(shm_id, NULL, 0);

    if (shm_ptr == (void *) -1) {
        perror("Child program | shmat");
        exit(1);
    } else {
        printf("Child program |  Shared memory attached at address: %p\n", shm_ptr);
    }
}

void register_signal_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGUSR1, &sa, NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <shm_id>\n", argv[0]);
        return 1;
    }

    attach_shared_memory(argv[1]);
    register_signal_handler();

    while (1) {
        pause();
    }

    shmdt(shm_ptr);
    return 0;
}