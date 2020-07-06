#define _SVID_SOURCE 1

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
	// ftok generates unique key
	key_t key = ftok("shmfile", 65);
	// shmget returns an identifier in shmid
	int shmid = shmget(key, 10*sizeof(int), 0666|IPC_CREAT);

	int *arr = (int*)shmat(shmid, (void*)0, 0);
	for (int i=0;i<10;++i)	*(arr+i) = 10*i;
	/*
	for (int i=0;i<10;++i)	printf("%d=%d\n", i, *(arr+i));
	printf("\n");
	*/

	if (!fork()) {
		for (int i=0;i<5;++i)	*(arr+i) = i;
		
		for (int i=0;i<10;++i)	printf("arr[%d]=%d\n", i, *(arr+i));

		shmdt(arr);
		exit(0);
	}
	while (wait(NULL) != -1);

	shmdt(arr);
	shmctl(shmid, IPC_RMID, NULL);
	printf("We finish!\n");
	return 0;

}

