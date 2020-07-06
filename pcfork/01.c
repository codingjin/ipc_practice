#define _SVID_SOURCE 1

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define SIZE 10

extern int errno;

int main()
{

	int in=-1, out=-1;
	//int buf[10] = {-1};
	// ftok generates unique key
	key_t key = ftok("shmfile", 65);
	int shmid = shmget(key, 10*sizeof(int), 0666|IPC_CREAT);
	int *buf = (int*)shmat(shmid, (void*)0, 0);
	for (int i=0;i<10;++i)	*(buf+i) = -1;

	sem_t *mutex = sem_open("m1", O_CREAT | O_EXCL, 0777, 1);
	sem_t *empty = sem_open("e1", O_CREAT | O_EXCL, 0777, 10);
	sem_t *full = sem_open("f1", O_CREAT | O_EXCL, 0777, 0);

	if (mutex == SEM_FAILED) {
		fprintf(stderr, "mutext initialization failed!\n");
		exit(0);
	}
	if (empty == SEM_FAILED) {
		fprintf(stderr, "empty initialization failed!\n");
		exit(0);
	}
	if (full == SEM_FAILED) {
		fprintf(stderr, "full initialization failed!\n");
		exit(0);
	}
	int tt;
	if (!sem_getvalue(mutex, &tt))	printf("mutex[%d]\n", tt);
	else {
		printf("sem_getvalue(mutex, &tt) failed, errno=%d\n", errno);
		exit(0);
	}
	if (!sem_getvalue(empty, &tt))	printf("empty[%d]\n", tt);
	if (!sem_getvalue(full, &tt))	printf("full[%d]\n", tt);

	if (!fork()) {
		printf("I am Producer\n");
		// producer
		for (int i=0;i<500;++i) {
			sem_wait(empty);
			sem_wait(mutex);
			in = (in+1) % SIZE;
			buf[in] = i;
			//printf("buf[%d]=%d\n", in, i);
			sem_post(mutex);
			sem_post(full);
		}
		printf("Producer finished!\n");
		exit(0);
	}

	for (int i=0;i<5;++i) {
		if (!fork()) { // consumer[i]
			printf("I am consumer[%d]\n", i);
			for (int j=0;j<100;++j) {
				if (!sem_wait(full)) {
					if (!sem_wait(mutex)) {
						//out = (out+1) % SIZE;
						//printf("out:%d\n", buf[out]);

						for (int pos=0;pos<10;++pos) {
							if (buf[pos] != -1) {
								printf("out[%d]:%d\n", i, buf[pos]);
								buf[pos] = -1;
								break;
							}
						}

						if (sem_post(mutex)) {
							fprintf(stderr, "failed in sem_post(mutex) in Consumer[%d]\n", i);
							exit(0);
						}
						if (sem_post(empty)) {
							fprintf(stderr, "failed in sem_post(empty) in Consumer[%d]\n", i);
							exit(0);
						}

					}else {
						fprintf(stderr, "failed in sem_wait(mutex) in consumer[%d]\n", i);
						exit(0);
					}
				}else {
					fprintf(stderr, "failed in sem_wait(full) in consumer[%d]\n", i);
					exit(0);
				}
			}
			printf("Consumer[%d] finished!\n", i);
			exit(0);
		}
	}

	while (wait(NULL) != -1);
	return 0;

}


