/* CS 519, FALL 2019: HW-1 
 * IPC using shared memory to perform matrix multiplication.
 * Feel free to extend or change any code or functions below.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

//Add all your global variables and definitions here.
#define MATRIX_SIZE 1000
#define CHILDREN_NUM 16

int **Matrix(int m, int n);
void freeMatrix(int **M, int m);
int correct(int *shmptr, int **A, int **B, int m, int n, int r);


/* Time function that calculates time between start and end */
double getdetlatimeofday(struct timeval *begin, struct timeval *end)
{
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char const *argv[])
{
	if(argc != 5) {
		fprintf(stderr, "Invalid command! Standard command is ./IPC-shmem integer1 integer2 integer3 integer4\nThese 4 integers are the dimensions for the 2 matrixes\n");
		return 1;
	}
	int m=atoi(argv[1]), m1=atoi(argv[2]), m2=atoi(argv[3]), n=atoi(argv[4]);
	if(m<1 || m1<1 || m2<1 || n<1 || m1!=m2 || m>MATRIX_SIZE || m1>MATRIX_SIZE || n>MATRIX_SIZE) {
		fprintf(stderr, "Invalid input arguments[m=%d, m1=%d, m2=%d, n=%d]\n", m, m1, m2, n);
		return 1;
	}
	
	int **A = Matrix(m, m1);
	int **B = Matrix(m2, n);

	int total=m*n, id, len=total/(CHILDREN_NUM+1), totalbytes=total*sizeof(int);
	int shmid = shmget(IPC_PRIVATE, totalbytes, IPC_CREAT | 0666);
	if(shmid < 0) {
		fprintf(stderr, "shmget fails!\n");
		return 1;
	}

	int *shmptr = (int*)shmat(shmid, NULL, 0);
	if(shmptr == (void*)-1) {
		fprintf(stderr, "shmat fails!\n");
		return 1;
	}
	memset(shmptr, 0, totalbytes);
	

	struct timeval tbeg, tend;
	// Time begin
	gettimeofday(&tbeg, NULL);
	pid_t pid;
	for(id=0;id<CHILDREN_NUM;++id) {
		pid = fork();
		if(pid == 0) { // child
			if(len != 0) {
				int beg=id*len, end=beg+len-1, i0=beg/n, j0=beg-n*i0, i1=end/n, j1=end-n*i1;
				if(i0==i1) {
					for(int k=0;k<m1;++k)	for(int j=j0;j<=j1;++j)	shmptr[beg+j-j0]+=A[i0][k]*B[k][j];//arr[j-j0]+=A[i0][k]*B[k][j];
				}else { // i0<i1
					int i, j, k;
					// i==i0 j=j0:m1-1
					for(k=0;k<m1;++k)	for(j=j0;j<m1;++j)	shmptr[beg+j-j0]+=A[i0][k]*B[k][j];//arr[j-j0]+=A[i0][k]*B[k][j];

					// i=i0+1:i1-1 j=0:m1-1
					for(k=0;k<m1;++k)	for(i=i0+1;i<i1;++i)	for(j=0;j<m1;++j)	shmptr[i*n+j]+=A[i][k]*B[k][j];//arr[i*n+j-beg]+=A[i][k]*B[k][j];

					// i==i1 j=0:j1
					for(k=0;k<m1;++k)	for(j=0;j<=j1;++j)	shmptr[i1*n+j]+=A[i1][k]*B[k][j];//arr[i1*n+j-beg]+=A[i1][k]*B[k][j];

				}
			}else { // len==0
				if(id<total) {
					int i=id/n, j=id-n*i;
					for(int k=0;k<m1;++k)	shmptr[id]+=A[i][k]*B[k][j];
				}
			}
			shmdt((void*)shmptr);
			freeMatrix(A, m);
			freeMatrix(B, m1);
			return 0;
		}else if(pid < 0) {
			fprintf(stderr, "fork fails!\n");
			return 1;
		}
	}

	// parent 
	int beg = 0;
	if(len) {
		beg=len*CHILDREN_NUM;
		int i0=beg/n, j0=beg-n*i0;
		for(int k=0;k<m1;++k)	for(int j=j0;j<n;++j)	shmptr[i0*n+j]+=A[i0][k]*B[k][j];
		for(int k=0;k<m1;++k)	for(int i=i0+1;i<m;++i)	for(int j=0;j<n;++j)	shmptr[i*n+j]+=A[i][k]*B[k][j];
	}

	// now parent begins to make sure all computation finishes
	while(wait(NULL)>0);
	// Time end
	gettimeofday(&tend, NULL);
	printf("Time taken:%lf\n", getdetlatimeofday(&tbeg, &tend));

	printf("Now check the correctness of the computation\n");
	if(correct(shmptr, A, B, m, n, m1))	printf("The Matrix Multiplication is Correct!\n");
	else								printf("The Matrix Multiplication is Incorrect!\n");

	freeMatrix(A, m);
	freeMatrix(B, m1);
	shmdt((void*)shmptr);
	shmctl(shmid, IPC_RMID, NULL);
   return 0;
}

int **Matrix(int m, int n) {
	int **M = (int**)calloc(m, sizeof(int*));
	if(M == NULL) {
		fprintf(stderr, "calloc for Matrx[%d][%d] fails!\n", m, n);
		exit(1);
	}
	for(int i=0;i<m;++i) {
		M[i] = (int*)calloc(n, sizeof(int));
		if(M[i] == NULL) {
			fprintf(stderr, "calloc for Matrx[%d] fails!\n", i);
			exit(1);
		}
		for(int j=0;j<n;++j)	M[i][j] = rand()%3;
	}
	return M;
}

void freeMatrix(int **M, int m) {
	for(int i=0;i<m;++i)	free(M[i]);
	free(M);
}

int correct(int *shmptr, int **A, int **B, int m, int n, int r) {
	int flag = 1;
	int **M = (int**)calloc(m, sizeof(int*));
	if(M == NULL) {
		fprintf(stderr, "calloc for correct() fails!\n");
		exit(1);
	}
	for(int i=0;i<m;++i) {
		M[i] = (int*)calloc(n, sizeof(int));
		if(M[i] == NULL) {
			fprintf(stderr, "calloc for M[%d] fails!\n", i);
			exit(1);
		}
	}
	for(int k=0;k<r;++k)	for(int i=0;i<m;++i)	for(int j=0;j<n;++j)	M[i][j]+=A[i][k]*B[k][j];
	for(int i=0;i<m;i++) {
		for(int j=0;j<n;++j) {
			if(M[i][j] != shmptr[i*n+j]) {
				flag = 0;
				break;
			}
		}
		if(flag==0)	break;
	}
	for(int i=0;i<m;++i)	free(M[i]);
	free(M);
	return flag;
}

