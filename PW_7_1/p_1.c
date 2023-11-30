//
// Created by vboxuser on 11/30/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define MAX_BUFFER_SIZE 1024

int shm_id;
void *shm_ptr;
pid_t child_pid;

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, shm_ptr, sizeof(int));
        printf("Parent program | Sum: %d\n", sum);
    }
}

void create_shared_memory() {
    shm_id = shmget(IPC_PRIVATE, MAX_BUFFER_SIZE, IPC_CREAT | 0666);
    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *) -1) {
        perror("Parent program | shmat");
        exit(1);
    }
}

void destroy_shared_memory() {
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);
}

void run_child_process() {
    char shm_id_str[10];
    sprintf(shm_id_str, "%d", shm_id);
    execlp("./p_2", "p_2", shm_id_str, NULL);
    perror("execlp");
    exit(1);
}

void handle_parent_process() {
    while (1) {
        int count, i, num;
        printf("Parent program | Enter count for sum or enter 0 to exit: ");
        scanf("%d", &count);
        if (count <= 0) {
            break;
        }
        for (i = 0; i < count; i++) {
            printf("Parent program | Enter your num : ");
            scanf("%d", &num);
            memcpy(shm_ptr + i * sizeof(int), &num, sizeof(int));
        }
        int end_marker = 0;
        memcpy(shm_ptr + count * sizeof(int), &end_marker, sizeof(int));
        kill(child_pid, SIGUSR1);
        pause();
    }
    kill(child_pid, SIGTERM);
    waitpid(child_pid, NULL, 0);
}

int main(int argc, char *argv[]) {
    create_shared_memory();
    signal(SIGUSR1, signal_handler);
    child_pid = fork();
    if (child_pid == 0) {
        run_child_process();
    } else {
        handle_parent_process();
        destroy_shared_memory();
    }
    return 0;
}