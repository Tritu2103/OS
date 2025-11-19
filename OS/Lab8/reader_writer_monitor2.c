#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#define R 2
#define DELAY	sleep(rand() % R);

int readerCount;		// số reader đang đọc
int writerCount;		// có writer đang viết hay không (0/1)
int waitingReaders;		// số reader đang chờ
int waitingWriters;		// số writer đang chờ

pthread_cond_t canRead;		// điều kiện cho reader được đọc
pthread_cond_t canWrite;	// điều kiện cho writer được ghi
pthread_mutex_t mutex;		// mutex để đồng bộ

void *reader(void *arg);
void *writer(void *arg);
void beginRead(int i);
void beginWrite(int i);
void endRead(int i);
void endWrite(int i);
void readData(int i);
void writeData(int i);

int main() {
	unsigned int i, noReaderThreads, noWriterThreads;
	pthread_t readerThread[100], writerThread[100];
	
	// Khởi tạo
	readerCount = 0;
	writerCount = 0;
	waitingReaders = 0;
	waitingWriters = 0;
	pthread_cond_init(&canRead, NULL);
	pthread_cond_init(&canWrite, NULL);
	pthread_mutex_init(&mutex, NULL);
		
	noReaderThreads = 10; assert(noReaderThreads < 100);
	noWriterThreads = 3;  assert(noWriterThreads < 100);
 
	for (i = 0; i < noWriterThreads; i++) 
		pthread_create(&writerThread[i], NULL, writer, (void *)(size_t)i);
	
	for (i = 0; i < noReaderThreads; i++)
		pthread_create(&readerThread[i], NULL, reader, (void *)(size_t)i);
	
	for (i = 0; i < noWriterThreads; i++)
		pthread_join(writerThread[i], NULL);

	for (i = 0; i < noReaderThreads; i++)
		pthread_join(readerThread[i], NULL); 

	return 0;
}

void *writer(void *arg) {
	int id = (int)(size_t)arg;

	DELAY;

	beginWrite(id);
	writeData(id);	// ghi dữ liệu
	endWrite(id);

	return NULL;
}

void *reader(void *arg) { 
	int id = (int)(size_t)arg;

	DELAY;

	beginRead(id);
	readData(id);	// đọc dữ liệu
	endRead(id);

	return NULL;
}

void beginRead(int i) {
	pthread_mutex_lock(&mutex);

	// Nếu có writer đang hoạt động hoặc writer đang đợi
	if (writerCount == 1 || waitingWriters > 0) {
		waitingReaders++;
		pthread_cond_wait(&canRead, &mutex);
		waitingReaders--;
	}

	readerCount++;
	pthread_mutex_unlock(&mutex);
}

void endRead(int i) {
	pthread_mutex_lock(&mutex);
	if (--readerCount == 0)
		pthread_cond_signal(&canWrite); // cho writer vào nếu không còn reader
	pthread_mutex_unlock(&mutex);
}

void beginWrite(int i) {
	pthread_mutex_lock(&mutex);

	if (writerCount == 1 || readerCount > 0) {
		waitingWriters++;
		pthread_cond_wait(&canWrite, &mutex);
		waitingWriters--;
	}

	writerCount = 1;
	pthread_mutex_unlock(&mutex);
}

void endWrite(int i) {
	pthread_mutex_lock(&mutex);
	writerCount = 0;

	// Ưu tiên reader nếu có reader đang đợi
	if (waitingReaders > 0)
		pthread_cond_broadcast(&canRead);
	else
		pthread_cond_signal(&canWrite);

	pthread_mutex_unlock(&mutex);
}

void readData(int i) {
	printf("Reader %d is reading\n", i);
	DELAY;
	printf("Reader %d is done with the reading\n", i);
}

void writeData(int i) {
	printf("Writer %d is writing\n", i);
	DELAY;
	printf("Writer %d is done with the writing\n", i);
}
