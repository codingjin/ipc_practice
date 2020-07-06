#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define SIZE 10

extern int errno;

int main()
{
	sem_unlink("m1");
	sem_unlink("e1");
	sem_unlink("f1");
	/*
	sem_t *mutex = sem_open("m1", O_CREAT | O_EXCL, 0777, 1);
	sem_t *empty = sem_open("e1", O_CREAT | O_EXCL, 0777, 10);
	sem_t *full = sem_open("f1", O_CREAT | O_EXCL, 0777, 0);
	*/
	return 0;

}


