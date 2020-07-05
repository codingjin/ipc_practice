all: IPC-shmem

IPC-shmem: IPC-shmem.c
	gcc -oIPC-shmem IPC-shmem.c -Wall -Werror -std=c11 -fsanitize=address -D_SVID_SOURCE -O4

clean:
	rm -rf IPC-shmem
