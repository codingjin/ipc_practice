all: IPC-pipe

IPC-pipe: IPC-pipe.c
	gcc -oIPC-pipe IPC-pipe.c -Wall -Werror -std=c11 -fsanitize=address -D_SVID_SOURCE -O4

clean:
	rm -rf IPC-pipe
