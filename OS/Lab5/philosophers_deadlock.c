#include <stdio.h>    
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define N 5
#define R 3
#define TRUE 1
#define LEFT(i)	(i)
#define RIGHT(i) ((i + 1) % N)

pthread_mutex_t chopstick[N]; // Mỗi chiếc đũa là một mutex

void* philosopher(void *pNo) { 
	int i = *(int*)pNo; // ID của triết gia

	while (TRUE) {
		// Nghĩ
		printf("Triết gia %d đang suy nghĩ...\n", i);
		sleep(rand() % R);

		// Đói
		printf("Triết gia %d đang đói và cố gắng lấy đũa...\n", i);

		// Lấy đũa bên trái
		pthread_mutex_lock(&chopstick[LEFT(i)]);
		printf("Triết gia %d đã lấy đũa %d (trái)\n", i, LEFT(i));

		// Lấy đũa bên phải
		pthread_mutex_lock(&chopstick[RIGHT(i)]);
		printf("Triết gia %d đã lấy đũa %d (phải) và bắt đầu ăn\n", i, RIGHT(i));

		// Ăn
		sleep(rand() % R);

		// Thả đũa
		pthread_mutex_unlock(&chopstick[LEFT(i)]);
		pthread_mutex_unlock(&chopstick[RIGHT(i)]);
		printf("Triết gia %d đã đặt đũa xuống\n", i);
	}
	return NULL;
}

int main() {
	pthread_t tid[N];
	int philosopherID[N], i;

	// Khởi tạo mutex cho từng chiếc đũa
	for (i = 0; i < N; i++) {
		pthread_mutex_init(&chopstick[i], NULL);
	}

	// Tạo thread cho các triết gia
	for (i = 0; i < N; i++) {
		philosopherID[i] = i;
		pthread_create(&tid[i], NULL, philosopher, &philosopherID[i]);
	}

	// Chờ tất cả triết gia kết thúc (sẽ không bao giờ kết thúc do vòng lặp vô hạn)
	for (i = 0; i < N; i++) {
		pthread_join(tid[i], NULL);
	}

	// Hủy mutex (sẽ không chạy do vòng lặp vô hạn)
	for (i = 0; i < N; i++) {
		pthread_mutex_destroy(&chopstick[i]);
	}

	return 0;
}
