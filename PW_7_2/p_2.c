//
// Created by vboxuser on 11/30/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

typedef struct {
    int shm_id;
    void *shm_ptr;
    int sem_id;
} SharedData;

SharedData sharedData;

void P(int sem_id) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = SEM_UNDO;
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop P failed");
        exit(EXIT_FAILURE);
    }
}

void V(int sem_id) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = SEM_UNDO;
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop V failed");
        exit(EXIT_FAILURE);
    }
}

void signal_handler(int sig) {
    printf("Client: Signal handler received signal %d\n", sig);
    if (sig == SIGUSR1) {
        int sum = 0, i = 0, val;

        printf("Client: Locking semaphore before reading data...\n");
        P(sharedData.sem_id);

        do {
            memcpy(&val, (int *)sharedData.shm_ptr + i, sizeof(int));
            if (val == 0) break;
            sum += val;
            i++;
        } while (1);

        memcpy(sharedData.shm_ptr, &sum, sizeof(int));

        printf("Client: Releasing semaphore after reading...\n");
        V(sharedData.sem_id);
        printf("Client: Sending signal back to server...\n");
        kill(getppid(), SIGUSR1);
    }
}

void connect_shared_memory(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <shm_id> <sem_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Client: Connecting to shared memory...\n");
    sharedData.shm_id = atoi(argv[1]);
    sharedData.sem_id = atoi(argv[2]);

    sharedData.shm_ptr = shmat(sharedData.shm_id, NULL, 0);
    if (sharedData.shm_ptr == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    printf("Client: Connecting to semaphore...\n");
    signal(SIGUSR1, signal_handler);
}

void run_client() {
    while (1) {
        pause();
    }
}

int main(int argc, char *argv[]) {
    connect_shared_memory(argc, argv);
    run_client();
    shmdt(sharedData.shm_ptr);
    return 0;
}
