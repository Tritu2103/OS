#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define up(sem) sem_post(sem)
#define down(sem) sem_wait(sem)
#define DELAY(R) sleep(rand() % R)
#define MAX_CLIENTS 25

void *client(void *);
void *barber(void *);

sem_t clients;
sem_t barbers;
sem_t mutex, seatBelt;
int waiting = 0, numChairs, numClients;
int allDone = 0;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Num clients> <Num Chairs>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    numClients = atoi(argv[1]);
    numChairs = atoi(argv[2]);

    if (numClients > MAX_CLIENTS) {
        printf("The maximum number of clients is %d.\n", MAX_CLIENTS);
        exit(EXIT_FAILURE);
    }

    pthread_t btid;
    pthread_t ctid[MAX_CLIENTS];
    int clientID[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++)
        clientID[i] = i;

    sem_init(&clients, 0, 0);
    sem_init(&barbers, 0, 0);
    sem_init(&mutex, 0, 1);
    sem_init(&seatBelt, 0, 0);

    pthread_create(&btid, NULL, barber, NULL);

    for (int i = 0; i < numClients; i++)
        pthread_create(&ctid[i], NULL, client, (void *)&clientID[i]);

    for (int i = 0; i < numClients; i++)
        pthread_join(ctid[i], NULL);

    allDone = 1;
    up(&clients);
    pthread_join(btid, NULL);

    return 0;
}

void *client(void *number) {
    int num = *(int *)number;
    DELAY(10);
    printf("Client %d arrives\n", num);

    down(&mutex);
    if (waiting < numChairs) {
        waiting++;
        up(&clients);
        printf("Client %d is waiting for turn\n", num);
        up(&mutex);

        down(&barbers);
        printf("Client %d is having a haircut\n", num);
        down(&seatBelt);
        printf("Client %d is going home\n", num);
    } else {
        printf("Shop full. Client %d leaves\n", num);
        up(&mutex);
    }
    return NULL;
}

void *barber(void *junk) {
    while (!allDone) {
        printf("The barber is sleeping\n");
        down(&clients);

        if (!allDone) {
            down(&mutex);
            waiting--;
            up(&barbers);
            up(&mutex);

            printf("The barber is cutting hair\n");
            sleep(2);

            up(&seatBelt);
            printf("The barber has finished cutting hair.\n");
        } else {
            printf("The barber is going home for the day.\n");
        }
    }
    return NULL;
}
