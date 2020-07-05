/* CS 519, FALL 2019: HW-1 
 * IPC using pipes to perform matrix multiplication.
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
#define BUF_SIZE 4096
#define CHILDREN_NUM 16
//#define CHILDREN_NUM 25
#define PACKAGE_SIZE 128
#define RESULT_UNIT_SIZE 32
#define HEAD_SIZE 8
#define DELIMITER " "

int **Matrix(int m, int n);
int **Matrix0(int m, int n);
void printMatrix(int **M, int m, int n);
void freeMatrix(int **M, int m);
//void MatrixMultiplication(int **M, int **A, int **B, int m, int n, int k);
//int MatrixCompare(int **M, int **N, int m, int n);
int correct(int **C, int **A, int **B, int m, int n, int k);

/* Time function that calculates time between start and end */
double getdetlatimeofday(struct timeval *begin, struct timeval *end) {
    return (end->tv_sec + end->tv_usec * 1.0 / 1000000) -
           (begin->tv_sec + begin->tv_usec * 1.0 / 1000000);
}

int main(int argc, char const *argv[])
{
	if(argc != 5) {
		fprintf(stderr, "Invalid command! Standard command is ./IPC-pipe integer1 integer2 integer3 integer4\nThese 4 integers are the dimensions for the 2 matrixes\n");
		return 1;
	}
	int m=atoi(argv[1]), m1=atoi(argv[2]), m2=atoi(argv[3]), n=atoi(argv[4]);
	if(m<1 || m1<1 || m2<1 || n<1 || m1!=m2 || m>MATRIX_SIZE || m1>MATRIX_SIZE || n>MATRIX_SIZE) {
		fprintf(stderr, "Invalid input arguments[m=%d, m1=%d, m2=%d, n=%d]\n", m, m1, m2, n);
		return 1;
	}

	int pipefd[2];
	if(pipe(pipefd) < 0) {
		fprintf(stderr, "pipe() failed!\n");
		return 1;
	}
	int **A = Matrix(m, m1);
	int **B = Matrix(m2, n);
	
	pid_t fid;
	int total = m*n;
	int len = total / (CHILDREN_NUM+1);
	struct timeval tbeg, tend;
	// Time begin
	gettimeofday(&tbeg, NULL);
	int id;
	for(id=0;id<CHILDREN_NUM;++id) {
		fid = fork();
		if(fid == 0) {
			// child
			close(pipefd[0]);
			if(len!=0) {
				char result_unit[RESULT_UNIT_SIZE], head[HEAD_SIZE];
				char *resultbuf, *package;
				//resultbuf = (char*)malloc(BUF_SIZE);
				resultbuf = (char*)calloc(BUF_SIZE, 1);
				if(resultbuf == NULL) {
					fprintf(stderr, "calloc fails!\n");
					return 1;
				}
				//resultbuf[0] = 0;
				//package = (char*)malloc(BUF_SIZE);
				package = (char*)calloc(BUF_SIZE, 1);
				if(package == NULL) {
					fprintf(stderr, "calloc fails!\n");
					return 1;
				}
				int k, i, j, count=0;
				int beg=len*id, end=len*(id+1) - 1;
				int i0=beg/n, j0=beg-n*i0, i1=end/n, j1=end-n*i1;
				//int *arr = (int*)malloc(len*sizeof(int));
				int *arr = (int*)calloc(len, sizeof(int));
				if(arr == NULL)	{
					fprintf(stderr, "calloc for arr fails!\n");
					return 1;
				}
				//memset(arr, 0, len*sizeof(int));
				if(i0<i1) {
					// C[i0][j0:n-1]
					//for(k=0;k<m1;++k)	for(j=j0;j<n;++j)	C[i0][j] += A[i0][k]*B[k][j];
					for(k=0;k<m1;++k)	for(j=j0;j<n;++j)	arr[j-j0] += A[i0][k]*B[k][j];
					int arrpos = 0;
					for(j=j0;j<n;++j) {
						//sprintf(result_unit, "%d %d %d ", i0, j, C[i0][j]);
						sprintf(result_unit, "%d %d %d ", i0, j, arr[arrpos]);
						++arrpos;
						strcat(resultbuf, result_unit);
						++count;
						if(count == PACKAGE_SIZE) {
							sprintf(head, "%d ", PACKAGE_SIZE);
							strcpy(package, head);
							strcat(package, resultbuf);
							write(pipefd[1], package, BUF_SIZE);
							count = 0;
							resultbuf[0] = 0;
						}
					}
					// C[i0+1:i1-1][0:n-1]
					for(k=0;k<m1;++k)	for(i=i0+1;i<i1;++i)	for(j=0;j<n;++j)	arr[i*n+j-beg]+=A[i][k]*B[k][j]; //C[i][j] += A[i][k]*B[k][j];
					for(i=i0+1;i<i1;++i) {
						for(j=0;j<n;++j) {
							//sprintf(result_unit, "%d %d %d ", i, j, C[i][j]);
							sprintf(result_unit, "%d %d %d ", i, j, arr[arrpos]/*arr[i*n+j-beg]*/);
							++arrpos;
							strcat(resultbuf, result_unit);
							++count;
							if(count == PACKAGE_SIZE) {
								sprintf(head, "%d ", PACKAGE_SIZE);
								strcpy(package, head);
								strcat(package, resultbuf);
								write(pipefd[1], package, BUF_SIZE);
								count = 0;
								resultbuf[0] = 0;
							}
						}
					}

					// C[i1][0:j1]
					for(k=0;k<m1;++k)	for(j=0;j<=j1;++j)	arr[i1*n+j-beg]+=A[i1][k]*B[k][j];
					for(j=0;j<=j1;++j) {
						sprintf(result_unit, "%d %d %d ", i1, j, arr[arrpos]/*C[i1][j]*/);
						++arrpos;
						strcat(resultbuf, result_unit);
						++count;
						if(count == PACKAGE_SIZE) {
							sprintf(head, "%d ", PACKAGE_SIZE);
							strcpy(package, head);
							strcat(package, resultbuf);
							write(pipefd[1], package, BUF_SIZE);
							count = 0;
							resultbuf[0] = 0;
						}
					}
				}else { // i0==i1
					for(k=0;k<m1;++k)	for(j=j0;j<=j1;++j)	arr[j-j0]+=A[i0][k]*B[k][j]; //C[i0][j]+=A[i0][k]*B[k][j];
					for(j=j0;j<=j1;++j) {
						sprintf(result_unit, "%d %d %d ", i0, j, arr[j-j0]/*C[i0][j]*/);
						strcat(resultbuf, result_unit);
						++count;
						if(count == PACKAGE_SIZE) {
							sprintf(head, "%d ", PACKAGE_SIZE);
							strcpy(package, head);
							strcat(package, resultbuf);
							write(pipefd[1], package, BUF_SIZE);
							count = 0;
							resultbuf[0] = 0;
						}
					}
				}
				if(count) {
					sprintf(head, "%d ", count);
					strcpy(package, head);
					strcat(package, resultbuf);
					write(pipefd[1], package, BUF_SIZE);
				}
				free(resultbuf);
				free(package);
				free(arr);
			}else { // len==0
				if(id<total) {
					int tmp = 0;
					char outbuf[BUF_SIZE];
					int i=id/n, j=id-n*i;
					for(int k=0;k<m1;++k)	tmp+=A[i][k]*B[k][j];
					sprintf(outbuf, "1 %d %d %d ", i, j, tmp);
					write(pipefd[1], outbuf, BUF_SIZE);
				}
			}
			close(pipefd[1]);
			freeMatrix(A, m);
			freeMatrix(B, m2);
			return 0;
		}else if(fid < 0) {
			fprintf(stderr, "fork() fails!\n");
			return 1;
		}
	}

	// parent
	close(pipefd[1]);
	int **C = Matrix0(m, n);
	if(len != 0) { // now id represents this parent! It is the time for parent to do his own task!
		int k, i, j;
		int beg=len*id; // Since it is the last process, so will finish all, which means end=total-1;
		int i0=beg/n, j0=beg-n*i0;
		// C[i0][j0:n-1]
		for(k=0;k<m1;++k)	for(j=j0;j<n;++j)	C[i0][j] += A[i0][k]*B[k][j];
		// finish the rest
		for(k=0;k<m1;++k)	for(i=i0+1;i<m;++i)	for(j=0;j<n;++j)	C[i][j] += A[i][k]*B[k][j];
	}

	// parent gets the results from pipefd[0]
	int i,j,num;
	char inbuf[BUF_SIZE], *ptr;
	while(read(pipefd[0], inbuf, BUF_SIZE) > 0) {
		//printf("Result:%s\n", inbuf);
		ptr = strtok(inbuf, DELIMITER);
		num = atoi(ptr);
		while(num) {
			ptr = strtok(NULL, DELIMITER);
			i = atoi(ptr);
			ptr = strtok(NULL, DELIMITER);
			j = atoi(ptr);
			ptr = strtok(NULL, DELIMITER);
			C[i][j] = atoi(ptr);
			--num;
		}
	}
	
	// Time end
	gettimeofday(&tend, NULL);
	printf("Time taken:%lf\n", getdetlatimeofday(&tbeg, &tend));
	close(pipefd[0]);

	printf("Now check the correctness of the computation\n");
	if(correct(C, A, B, m, n, m2))	printf("The Matrix Multiplication is correct!\n");
	else							printf("The Matrix Multiplication is incorrect!\n");
	
	freeMatrix(A, m);
	freeMatrix(B, m2);
	freeMatrix(C, m);
	return 0;
}


int **Matrix(int m, int n) {
	//int **M = (int**)malloc(m * sizeof(int*));
	int **M = (int**)calloc(m, sizeof(int*));
	if(M == NULL) {
		fprintf(stderr, "Fail to calloc for Matrix\n");
		exit(1);
	}
	for(int i=0;i<m;++i) {
		M[i] = (int*)calloc(n, sizeof(int));
		if(M[i] == NULL) {
			fprintf(stderr, "Fail to calloc for M[%d]\n", i);
			exit(1);
		}
		for(int j=0;j<n;++j)	M[i][j] = rand() % 3;
	}
	return M;
}

int **Matrix0(int m, int n) {
	int **M = (int**)calloc(m, sizeof(int*));
	if(M == NULL) {
		fprintf(stderr, "Fail to calloc for Matrix\n");
		exit(1);
	}
	for(int i=0;i<m;++i) {
		M[i] = (int*)calloc(n, sizeof(int));
		if(M[i] == NULL) {
			fprintf(stderr, "Fail to calloc for M[%d]\n", i);
			exit(1);
		}
	}
	return M;
}

void freeMatrix(int **M, int m) {
	for(int i=0;i<m;++i)	free(M[i]);
	free(M);
}

void printMatrix(int **M, int m, int n) {
	for(int i=0;i<m;++i) {
		for(int j=0;j<n;++j)	printf("%d ", M[i][j]);
		printf("\n");
	}
	printf("\n\n");
}
/*
void MatrixMultiplication(int **M, int **A, int **B, int m, int n, int k) {
	for(int i=0;i<m;++i)
		for(int j=0;j<n;++j) {
			M[i][j] = 0;
			for(int kk=0;kk<k;++kk)	M[i][j] += A[i][kk]*B[kk][j];
		}
}
*/
/*
int MatrixCompare(int **M, int **N, int m, int n) {
	for(int i=0;i<m;++i)	for(int j=0;j<n;++j)	if(M[i][j] != N[i][j])	return 0;
	return 1;
}
*/

int correct(int **C, int **A, int **B, int m, int n, int r) {
	int **M = Matrix0(m, n);
	int flag = 1;
	for(int k=0;k<r;++k)	for(int i=0;i<m;++i)	for(int j=0;j<n;++j)	M[i][j]+=A[i][k]*B[k][j];
	for(int i=0;i<m;++i) {
		for(int j=0;j<n;++j)
			if(C[i][j] != M[i][j]) {
				flag = 0;
				break;
			}

		if(!flag)	break;
	}
	
	for(int i=0;i<m;++i)	free(M[i]);
	free(M);
	return flag;
}

