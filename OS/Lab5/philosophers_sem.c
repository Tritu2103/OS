#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#define N 5  // Số lượng triết gia
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define LEFT(i) (i + N - 1) % N
#define RIGHT(i) (i + 1) % N
#define R 3  // Khoảng thời gian tối đa cho ăn/suy nghĩ

sem_t mutex;  // Semaphore bảo vệ thao tác thay đổi trạng thái
sem_t S[N];   // Semaphore cho từng triết gia

int state[N];  // Trạng thái của từng triết gia
int phil_num[N];  // ID của triết gia

void *philosopher(void *pNum);
void take_chopsticks(int i);
void put_chopsticks(int i);
void test(int i);
void think(int i);
void eat(int i);

int main() {
    int i;
    pthread_t thread_id[N];

    // Khởi tạo semaphore
    sem_init(&mutex, 0, 1);  // Semaphore mutex để điều phối truy cập
    for (i = 0; i < N; i++)
        sem_init(&S[i], 0, 0);  // Semaphore cho từng triết gia

    // Tạo thread cho mỗi triết gia
    for (i = 0; i < N; i++) {
        phil_num[i] = i;
        pthread_create(&thread_id[i], NULL, philosopher, &phil_num[i]);
    }

    // Chờ các thread kết thúc (sẽ không bao giờ kết thúc)
    for (i = 0; i < N; i++)
        pthread_join(thread_id[i], NULL);

    return 0;
}

// Hàm thực hiện vòng đời của một triết gia
void *philosopher(void *pNum) {
    int i = *(int *)pNum;

    while (1) {
        think(i);          // Triết gia suy nghĩ
        take_chopsticks(i); // Cố gắng lấy đũa để ăn
        eat(i);           // Triết gia ăn
        put_chopsticks(i); // Thả đũa và quay lại suy nghĩ
    }
}

// Triết gia lấy đũa
void take_chopsticks(int i) {
    sem_wait(&mutex);  // Đảm bảo chỉ một triết gia thay đổi trạng thái tại một thời điểm
    state[i] = HUNGRY;
    printf("Philosopher %d is hungry\n", i + 1);

    test(i);  // Kiểm tra xem có thể ăn không

    sem_post(&mutex);
    sem_wait(&S[i]);  // Chờ đến khi có thể ăn (nếu bị chặn)
}

// Kiểm tra xem triết gia i có thể ăn không
void test(int i) {
    if (state[i] == HUNGRY &&
        state[LEFT(i)] != EATING &&
        state[RIGHT(i)] != EATING) {
        state[i] = EATING;
        printf("Philosopher %d takes chopsticks %d and %d\n", i + 1, LEFT(i) + 1, i + 1);
        printf("Philosopher %d is eating\n", i + 1);
        sem_post(&S[i]);  // Mở khóa semaphore để triết gia bắt đầu ăn
    }
}

// Triết gia thả đũa xuống
void put_chopsticks(int i) {
    sem_wait(&mutex);
    state[i] = THINKING;
    printf("Philosopher %d puts down chopsticks %d and %d\n", i + 1, LEFT(i) + 1, i + 1);
    printf("Philosopher %d is thinking\n", i + 1);

    test(LEFT(i));  // Kiểm tra xem triết gia bên trái có thể ăn không
    test(RIGHT(i)); // Kiểm tra xem triết gia bên phải có thể ăn không

    sem_post(&mutex);
}

// Triết gia suy nghĩ
void think(int i) {
    printf("Philosopher %d is thinking\n", i + 1);
    sleep(rand() % R);
}

// Triết gia ăn
void eat(int i) {
    sleep(rand() % R);
}
