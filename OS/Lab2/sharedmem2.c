// Producer-Consumer using shared memory (concurrent processes)
// Compile with: gcc producer_consumer.c -o producer_consumer -lrt

#include <stdio.h>             
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

// Shared memory structure
struct mySharedMem {
    char buf[256];
} *pMem;

int main() {
    pid_t pid;
    int shmfd;

    // Create shared memory object
    shm_unlink("mySharedMemObject"); // Remove object if it exists
    shmfd = shm_open("mySharedMemObject", O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    assert(shmfd != -1);

    // Resize the memory region to store one struct instance
    assert(ftruncate(shmfd, sizeof(struct mySharedMem)) != -1);

    // Map the object into memory
    pMem = mmap(NULL, sizeof(struct mySharedMem), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    assert(pMem != MAP_FAILED);

    // Initialize shared data
    printf("Type a message to the child: ");
    fgets(pMem->buf, 255, stdin); // Get user's input (max 255 chars)

    // Create a child process
    pid = fork();
    if (pid == 0) { // Child process
        // Convert to uppercase
        for (int i = 0; i < strlen(pMem->buf); i++) {
            pMem->buf[i] = toupper(pMem->buf[i]);
        }

        // Unmap memory before exiting
        munmap(pMem, sizeof(struct mySharedMem));
        return 0;
    }

    // Parent process, waiting for child to finish
    wait(NULL);

    // Print the modified message
    printf("Message from the child: %s\n", pMem->buf);

    // Cleanup shared memory
    munmap(pMem, sizeof(struct mySharedMem));
    shm_unlink("mySharedMemObject");

    return 0;
}
