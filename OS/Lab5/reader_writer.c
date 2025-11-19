#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h> // Thư viện cho sem_open

#define up(sem) sem_post(sem)
#define down(sem) sem_wait(sem)
#define R 2
#define DELAY sleep(rand() % R);

sem_t *readCountAccess;   // Semaphore bảo vệ readCount
sem_t *databaseAccess;    // Semaphore bảo vệ database
int readCount = 0;        // Số lượng readers đang đọc

void *reader(void *arg);
void *writer(void *arg);
void readData(int i);
void writeData(int i);

int main() {
    unsigned int i, noReaderThreads, noWriterThreads;
    pthread_t readerThread[100], writerThread[100];

    // Khởi tạo semaphore (dùng sem_open thay vì sem_init trên macOS)
    readCountAccess = sem_open("/readCountAccess", O_CREAT, 0644, 1);
    databaseAccess = sem_open("/databaseAccess", O_CREAT, 0644, 1);

    if (readCountAccess == SEM_FAILED || databaseAccess == SEM_FAILED) {
        perror("Semaphore initialization failed");
        exit(EXIT_FAILURE);
    }

    noReaderThreads = 10;
    assert(noReaderThreads < 100);
    noWriterThreads = 3;
    assert(noWriterThreads < 100);

    // Tạo thread cho writer
    for (i = 0; i < noWriterThreads; i++) {
        pthread_create(&writerThread[i], NULL, writer, (void *)(intptr_t)i);
    }

    // Tạo thread cho reader
    for (i = 0; i < noReaderThreads; i++) {
        pthread_create(&readerThread[i], NULL, reader, (void *)(intptr_t)i);
    }

    // Chờ tất cả writer kết thúc
    for (i = 0; i < noWriterThreads; i++) {
        pthread_join(writerThread[i], NULL);
    }

    // Chờ tất cả reader kết thúc
    for (i = 0; i < noReaderThreads; i++) {
        pthread_join(readerThread[i], NULL);
    }

    // Hủy semaphore trên macOS
    sem_close(databaseAccess);
    sem_close(readCountAccess);
    sem_unlink("/databaseAccess");
    sem_unlink("/readCountAccess");

    return 0;
}

// Writer ghi dữ liệu
void *writer(void *arg) {
    int id = (intptr_t)arg;
    DELAY;

    // 1. Yêu cầu quyền truy cập vào database
    down(databaseAccess);

    // 2. Ghi dữ liệu
    writeData(id);

    // 3. Mở khóa quyền truy cập database
    up(databaseAccess);

    return NULL;
}

// Reader đọc dữ liệu
void *reader(void *arg) {
    int id = (intptr_t)arg;
    DELAY;

    // 1. Yêu cầu quyền truy cập biến readCount
    down(readCountAccess);

    // 2. Tăng số lượng reader
    readCount++;

    // 3. Nếu là reader đầu tiên, khóa database
    if (readCount == 1) {
        down(databaseAccess);
    }

    // 4. Mở khóa biến readCount
    up(readCountAccess);

    // Đọc dữ liệu
    readData(id);

    // 5. Yêu cầu quyền truy cập biến readCount
    down(readCountAccess);

    // 6. Giảm số lượng readers
    readCount--;

    // 7. Nếu là reader cuối cùng, mở khóa database
    if (readCount == 0) {
        printf("Reader %d unlocks the database\n", id);
        up(databaseAccess);
    }

    // 8. Mở khóa biến readCount
    up(readCountAccess);

    return NULL;
}

// Reader đọc dữ liệu
void readData(int i) {
    printf("Reader %d is reading\n", i);
    DELAY;
    printf("Reader %d is done with the reading\n", i);
}

// Writer ghi dữ liệu
void writeData(int i) {
    printf("Writer %d is writing\n", i);
    DELAY;
    printf("Writer %d is done with the writing\n", i);
}
