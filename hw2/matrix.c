#ifndef _REENTRANT //always remember to put this thing when you program threads
  #define _REENTRANT
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define N 5
int mat_A [N][N] = {0};
int mat_B [N][N] = {0};
int  mat_C [N][N] = {0};
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int MAX_ROW_SUM = 0; //store maximum of all rows

struct thread_info {
  int pos;
  int value;
};

// prints an NxM matrix
void print_matrix(int n, int m, int mat[n][m])
{
  int i, j;
  for(i = 0; i< n; i++){
    for( j =0; j< m; j++){
      printf("%8d", mat[i][j]);
    }
    printf("\n");
  }
}

void* fill_matrixA(void* thread_info) {
  struct thread_info* t_info = ( struct thread_info *) thread_info;
  int idx = t_info -> pos;
  int val = t_info -> value;
  mat_A[idx/N][idx%N] = val;
}

void* fill_matrixB(void* thread_info) {
  struct thread_info* t_info = ( struct thread_info *) thread_info;
  int idx = t_info -> pos;
  int val = t_info -> value;
  mat_B[idx/N][idx%N] = val;
}

void* matrix_mult(void* thread_info) {
  struct thread_info* t_info = ( struct thread_info *) thread_info;
  int idx = t_info -> pos;
  for(int k = 0; k < N; k++) {
    mat_C[idx/N][idx%N] += mat_A[idx/N][k] * mat_B[k][idx%N]; 
  }
}

void* row_sum(void* thread_info) {
  struct thread_info* t_info = ( struct thread_info *) thread_info;
  int idx = t_info -> pos;
  int wait_time = t_info -> value;
  //if we don't lock, the MAX_ROW_SUM will be updated several times.
  pthread_mutex_lock(&mutex1);
  int temp = MAX_ROW_SUM;
  for(int i = 0; i < N; i++) {
    temp = temp - mat_C[idx][i]; 
  }
  printf("we will wait %d\n",wait_time);
  sleep(wait_time);
  if(temp < 0) {
    for(int i = 0; i < N; i++) {
      MAX_ROW_SUM += mat_C[idx][i];
    }
  }
  //printf("MAX_ROW_SUM is %d\n",MAX_ROW_SUM);
  pthread_mutex_unlock(&mutex1);
}


int main(int argc, char** argv) {
  pthread_t thread[N*N];
  int iret[N*N];
  int seed[N*N];
  struct thread_info t_info [N*N];

  //fill in matrix A
  for (int i = 0; i < N*N; i++) {
    //printf("what I have in ms now: %s\n",ms[i]);
    seed[i] = time(NULL)+i;
    int num_print = rand_r(&seed[i])%1000; //don't want it that large
    // printf("I have %d\n", num_print);
    t_info[i].pos = i;
    t_info[i].value = num_print;
    iret[i] = pthread_create (&thread[i], NULL, fill_matrixA, (void *) &t_info[i]);
    if(iret[i]) {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret[i]);
      exit(EXIT_FAILURE);
    }
  }
  for(int i = 0; i < N*N; i++) {
    //printf("\npthread_create() for thread %d returns: %d\n",i,iret[i]);
    pthread_join(thread[i], NULL);
  }//fill in matrix a ends

  
  //fill in matrix b
    for (int i = 0; i < N*N; i++) {
    //printf("what I have in ms now: %s\n",ms[i]);                           
    seed[i] = time(NULL)+i+N*N;
    int num_print = rand_r(&seed[i])%1000; //don't want it that large        
    // printf("I have %d\n", num_print);                                       
    t_info[i].pos = i;
    t_info[i].value = num_print;
    iret[i] = pthread_create (&thread[i], NULL, fill_matrixB, (void *) &t_info[
i]);
    if(iret[i]) {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret[i]);
      exit(EXIT_FAILURE);
    }
  }

  for(int i = 0; i < N*N; i++) {
    //printf("\npthread_create() for thread %d returns: %d\n",i,iret[i]);
    pthread_join(thread[i], NULL);
  }//fill in matrix b ends

  //print matrix A and matrix B
  printf("This is matrix A:\n");
  print_matrix(N,N,mat_A);
  printf("\nThis is matrix B:\n");
  print_matrix(N,N,mat_B);

  //compute matrix C
  for (int i = 0; i < N*N; i++) {                                          
    t_info[i].pos = i;
    iret[i] = pthread_create (&thread[i], NULL, matrix_mult, (void *) &t_info[i]);
    if(iret[i]) {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret[i]);
      exit(EXIT_FAILURE);
    }
  }

  for(int i = 0; i < N*N; i++) {
    //printf("\npthread_create() for thread %d returns: %d\n",i,iret[i]);
    pthread_join(thread[i], NULL);
  }

  //print matrix C
  printf("\nThis is matrix C:\n");
  print_matrix(N,N,mat_C);


  //compute the maximum of all rows in mat_C
  for (int i = 0; i < N; i++) {
    t_info[i].pos = i;
    seed[i] = time(NULL)+i+2*N*N;
    int num_print = rand_r(&seed[i])%10;
    t_info[i].value = num_print;
    iret[i] = pthread_create (&thread[i], NULL, row_sum, (void *) &t_info[i]);
    if(iret[i]) {
      fprintf(stderr,"Error - pthread_create() return code: %d\n",iret[i]);
      exit(EXIT_FAILURE);
    }
  }

  for(int i = 0; i < N; i++) {
    //printf("\npthread_create() for thread %d returns: %d\n",i,iret[i]); 
    pthread_join(thread[i], NULL);
  }

  printf("MAX_ROW_SUM is %d\n",MAX_ROW_SUM);
  exit(EXIT_SUCCESS);
  
}
