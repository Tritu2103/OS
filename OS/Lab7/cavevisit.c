#include<stdio.h>             
#include<pthread.h>
#include<semaphore.h>
#include<stdlib.h>
#include<unistd.h>

#define N 5		// Max visitors in the cave
#define MAXTHREADS 20
#define TRUE 1
#define FALSE 0
#define up(sem) sem_post(sem)
#define down(sem) sem_wait(sem)
#define DELAY(R) sleep(rand() % R);

sem_t visitors, mutex;
int noOfVisitors = 0;

void* caveVisitor(void* id) {
    int visitorID = *((int*)id);
    
    down(&visitors); // Wait until one's turn
    
    down(&mutex);
    noOfVisitors++; // Increase number of visitors in the cave
    printf("There are %d visitors in the cave now.\n", noOfVisitors);
    up(&mutex);

    // Entering the cave through a 1-man passage
    down(&mutex);
    printf("Visitor %d is entering... ", visitorID);
    DELAY(3); // It takes a while
    printf("now in the cave.\n");
    up(&mutex);

    // Contemplating
    printf("Visitor %d is contemplating... \n", visitorID);
    DELAY(3); // It takes a while
    printf("Visitor %d is finished.\n", visitorID);

    // Getting out through the 1-man passage
    down(&mutex);
    printf("Visitor %d is getting out... ", visitorID);
    DELAY(3); // It takes a while
    printf("Visitor %d is out of the cave.\n", visitorID);
    up(&mutex);

    down(&mutex);
    noOfVisitors--; // Decrease number of visitors in the cave
    up(&mutex);
    
    up(&visitors); // Allow next visitor
    return NULL;
}

int main() {
    pthread_t tid[MAXTHREADS];
    int visitorID[MAXTHREADS];
    int i;

    sem_init(&mutex, 0, 1);
    sem_init(&visitors, 0, N);
    
    for (i = 0; i < MAXTHREADS; i++) {
        visitorID[i] = i + 1;
        pthread_create(&tid[i], NULL, caveVisitor, &visitorID[i]);
        DELAY(3);
    }
    
    for (i = 0; i < MAXTHREADS; i++) {
        pthread_join(tid[i], NULL);
    }
    
    sem_destroy(&mutex);
    sem_destroy(&visitors);
    
    return 0;
}
