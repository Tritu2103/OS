#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define FALSE 0

int turn = 0;

int write_data(char *buf, size_t len, int pid)
{
    int fileExist, fd;
    char *file = "output.dat";

    while (turn != pid)
        ;

    fileExist = check_file_existence(file);
    if (fileExist == FALSE)
    {
        fd = open(file, O_CREAT | O_WRONLY, 0644);
        write(fd, buf, len);
        close(fd);
    }

    turn = 1 - pid;

    return 0;
}