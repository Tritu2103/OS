#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define N 5
#define R 4
#define TRUE 1

int flag[2] = {0, 0};
int turn;

int queue[N];
int inIdx = 0, outIdx = 0, count = 0;

void *producer(void *arg);
void *consumer(void *arg);
void produce_item(int *pItem);
void enter_item(int item);
int get_item();
void consume_item(int item);
void enter_critical_section(int process);
void leave_critical_section(int process);

int main()
{
    pthread_t producer_id, consumer_id;

    pthread_create(&producer_id, NULL, producer, NULL);
    pthread_create(&consumer_id, NULL, consumer, NULL);

    pthread_join(producer_id, NULL);
    pthread_join(consumer_id, NULL);

    return 1;
}

void *producer(void *arg)
{
    int item;
    printf("Producer\n");
    while (TRUE)
    {
        produce_item(&item);
        while (count == N)
        {
            sleep(1);
        }
        enter_critical_section(0);
        enter_item(item);
        leave_critical_section(0);
        sleep(rand() % R);
    }
    return NULL;
}

void *consumer(void *arg)
{
    int item;
    printf("Consumer\n");
    while (TRUE)
    {
        while (count == 0)
        {
            sleep(1);
        }
        enter_critical_section(1);
        item = get_item();
        leave_critical_section(1);
        consume_item(item);
        sleep(rand() % R);
    }
    return NULL;
}

void enter_critical_section(int id)
{
    int other = 1 - id;
    flag[id] = 1;
    turn = other;
    while (turn == other && flag[other] == 1)
        ;
}

void leave_critical_section(int id)
{
    flag[id] = 0;
}

void produce_item(int *pItem)
{
    (*pItem) = rand() % 100;
    printf("P: Produce item %d\n", *pItem);
}

void enter_item(int item)
{
    queue[inIdx] = item;
    inIdx = (inIdx + 1) % N;
    count++;
    printf("P: Enter item %d\n", item);
    if (count == N)
        printf("P: The queue is full\n");
}

int get_item()
{
    int item;
    item = queue[outIdx];
    outIdx = (outIdx + 1) % N;
    count--;
    printf("C: Get item %d\n", item);
    if (count == 0)
        printf("C: The queue is empty\n");

    return item;
}

void consume_item(int item)
{
    printf("C: Item %d is yum yum!\n", item);
}