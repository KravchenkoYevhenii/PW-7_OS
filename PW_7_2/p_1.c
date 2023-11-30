//
// Created by vboxuser on 11/30/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define SHM_SIZE 1024

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

typedef struct {
    int shm_id;
    void *shm_ptr;
    pid_t child_pid;
    int sem_id;
} SharedData;

SharedData sharedData;

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, sharedData.shm_ptr, sizeof(int));
        printf("Sum: %d\n", sum);
    }
}

void init_semaphore(int sem_id, int sem_value) {
    union semun argument;
    argument.val = sem_value;
    if (semctl(sem_id, 0, SETVAL, argument) == -1) {
        perror("semctl");
        exit(1);
    }
}

void P(int sem_id) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;
    if (semop(sem_id, operations, 1) == -1) {
        perror("semop P");
        exit(1);
    }
}

void V(int sem_id) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;
    if (semop(sem_id, operations, 1) == -1) {
        perror("semop V");
        exit(1);
    }
}

void create_shared_memory(SharedData *sharedData) {
    sharedData->shm_id = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    sharedData->shm_ptr = shmat(sharedData->shm_id, NULL, 0);

    if (sharedData->shm_ptr == (void *) -1) {
        perror("shmat");
        exit(1);
    }
}

void create_semaphore(SharedData *sharedData) {
    sharedData->sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sharedData->sem_id == -1) {
        perror("semget");
        exit(1);
    }

    init_semaphore(sharedData->sem_id, 1);
}

void cleanup(SharedData *sharedData) {
    shmdt(sharedData->shm_ptr);
    shmctl(sharedData->shm_id, IPC_RMID, NULL);
    semctl(sharedData->sem_id, 0, IPC_RMID, 0);
}

void run_child_process(SharedData *sharedData) {
    char shm_id_str[10];
    char sem_id_str[10];
    sprintf(shm_id_str, "%d", sharedData->shm_id);
    sprintf(sem_id_str, "%d", sharedData->sem_id);

    execlp("./p_2", "p_2", shm_id_str, sem_id_str, (char *)NULL);

    perror("execlp");
    exit(1);
}

void handle_input(SharedData *sharedData) {
    while (1) {
        int n, i, input;
        printf("Enter quantity of nums to sum (0 - exit): ");
        scanf("%d", &n);

        if (n <= 0) {
            break;
        }

        printf("Server: Locking semaphore before writing data...\n");
        P(sharedData->sem_id);

        for (i = 0; i < n; i++) {
            printf("Enter the num: ");
            scanf("%d", &input);
            memcpy((int *)sharedData->shm_ptr + i, &input, sizeof(int));
        }

        printf("Server: Releasing semaphore after writing...\n");
        V(sharedData->sem_id);

        printf("Server: Sending signal to child process...\n");
        kill(sharedData->child_pid, SIGUSR1);

        printf("Server: Waiting for response from child...\n");
        pause();
    }
}


int main() {

    create_shared_memory(&sharedData);
    create_semaphore(&sharedData);

    signal(SIGUSR1, signal_handler);

    printf("Server: Creating child process...\n");
    sharedData.child_pid = fork();

    if (sharedData.child_pid == 0) {
        run_child_process(&sharedData);
    } else {
        handle_input(&sharedData);

        kill(sharedData.child_pid, SIGTERM);
        waitpid(sharedData.child_pid, NULL, 0);
        cleanup(&sharedData);
    }

    return 0;
}
