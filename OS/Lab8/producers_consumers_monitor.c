#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define N 5
#define TRUE 1
#define DELAY(R)	sleep(rand() % R);

pthread_cond_t empty = PTHREAD_COND_INITIALIZER; 
pthread_cond_t full  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int queue[N];
int inIdx = 0, outIdx = 0, count = 0;

void produce_item(int* pItem) {
	(*pItem) = rand() % 100;
	printf("P: Produce item %d\n", *pItem);
}

void enter_item(int item) {
	queue[inIdx] = item;
	inIdx = (inIdx + 1) % N;
	count++;
	printf("P: Enter item %d\n", item);
	if (count == N) printf("P: The queue is full\n");
}

int get_item() {
	int item = queue[outIdx];
	outIdx = (outIdx + 1) % N;
	count--;
	printf("C: Get item %d\n", item);
	if (count == 0) printf("C: The queue is empty\n");
	return item;
}

void consume_item(int item) {
	printf("C: Item %d is yum yum!\n", item);
}

void * producer (void * arg) {
	int item;
	printf("P: Hello\n");
	while (TRUE) {
		produce_item(&item); // Tạo ra sản phẩm

		pthread_mutex_lock(&mutex);
		while (count == N)
			pthread_cond_wait(&empty, &mutex); // Chờ cho đến khi có chỗ trống

		enter_item(item); // Đưa vào hàng đợi

		pthread_cond_signal(&full); // Báo cho consumer rằng có item mới
		pthread_mutex_unlock(&mutex);

		DELAY(3);
	}
}

void * consumer (void * arg) {
	int item;
	printf("C: Hello\n");
	while (TRUE) {
		pthread_mutex_lock(&mutex);
		while (count == 0)
			pthread_cond_wait(&full, &mutex); // Chờ cho đến khi có item

		item = get_item(); // Lấy item khỏi hàng đợi

		pthread_cond_signal(&empty); // Báo cho producer rằng có chỗ trống
		pthread_mutex_unlock(&mutex);

		consume_item(item); // Tiêu thụ sản phẩm
		DELAY(3);
	}
}

int main(int argc, char * argv[]) {
	pthread_t pid, cid;

	pthread_create(&pid, NULL, producer, NULL);
	pthread_create(&cid, NULL, consumer, NULL);
	pthread_join(pid, NULL);
	pthread_join(cid, NULL);
	return 0;
}
