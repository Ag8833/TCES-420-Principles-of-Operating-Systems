#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define MATRIXDIMENSIONS 500
int THREADNUMBER = 0;
int matrixA[MATRIXDIMENSIONS][MATRIXDIMENSIONS];
int matrixB[MATRIXDIMENSIONS][MATRIXDIMENSIONS];
int matrixResult[MATRIXDIMENSIONS][MATRIXDIMENSIONS];

//Function to run a single thread
void singleThread()
{
   setMatrix();

   struct timespec start, finish;
   double elapsed;
   clock_gettime(CLOCK_MONOTONIC, &start);
   int i, j, k;

   for(i = 0; i < MATRIXDIMENSIONS; i++)
      for(j = 0; j < MATRIXDIMENSIONS; j++)
         for(k = 0; k < MATRIXDIMENSIONS; k++)
         {
	    matrixResult[i][j] += matrixA[i][k] * matrixB[k][j];
	 }

   /*
   printf("MATRIX A - \n");
   printMatrix(matrixA);
   printf("MATRIX B - \n");
   printMatrix(matrixB);
   printf("MATRIX RESULT - \n");
   printMatrix(matrixResult);
   */

   clock_gettime(CLOCK_MONOTONIC, &finish);
   elapsed = (finish.tv_sec - start.tv_sec);
   elapsed += (finish.tv_sec - start.tv_sec) / 1000000000.0;
   printf("TOTAL SINGLE THREAD TIME SPENT: %f seconds \n", elapsed);
}

//Function to initialize the matrix to random values, and all 0's for the result matrix.
setMatrix()
{
   srand(time(NULL));
   int i, j;

   for(i = 0; i < MATRIXDIMENSIONS; i++)
      for(j = 0; j < MATRIXDIMENSIONS; j++)
      {
         matrixA[i][j] = rand() % 10;
         matrixB[i][j] = rand() % 10;
         matrixResult[i][j] = 0;
      }
}

//Function to print the matrix.
void printMatrix(int myMatrix[MATRIXDIMENSIONS][MATRIXDIMENSIONS])
{
   int i, j;
   for(i = 0; i < MATRIXDIMENSIONS; i++)
      for(j = 0; j < MATRIXDIMENSIONS; j++)
      {
         printf("%d ", myMatrix[i][j]);
         if(j == MATRIXDIMENSIONS - 1)
            printf("\n\n");
      }
}

//Function to run multi threads, it splits the array based on size and number of threads.
static void * multiThread(void *slice)
{
   int i, j, k;
   int s = (int)slice;
   int from, to;
   if(s == 0)
   {
      from = 0;
      to = ((s+1) * MATRIXDIMENSIONS)/THREADNUMBER;
   }
   else
   {
      from = (s * MATRIXDIMENSIONS)/THREADNUMBER;
      to = ((s+1) * MATRIXDIMENSIONS)/THREADNUMBER;
   }

   for(i = from; i < to; i++)
      for(j = 0; j < MATRIXDIMENSIONS; j++)
         for(k = 0; k < MATRIXDIMENSIONS; k++)
         {
	    matrixResult[i][j] += matrixA[i][k] * matrixB[k][j];
	 }

   //printf("FOR SLICE %d: \nFROM IS %d, TO IS %d \n",s,from,to);
}

int main()
{
   //singleThread();
   printf("Enter the number of threads (1,2,4,8,16): ");
   scanf("%d",&THREADNUMBER);

   setMatrix();

   struct timespec start, finish;
   double elapsed;
   clock_gettime(CLOCK_MONOTONIC, &start);
   int i;

   pthread_t threads[THREADNUMBER];

   for(i = 0; i < THREADNUMBER; i++)
   {
      pthread_create(&threads[i], NULL, multiThread, (void *)i);
   }

   for(i = 0; i < THREADNUMBER; i++)
   {
      pthread_join(threads[i], NULL);
   }

   /*
   printf("MATRIX A - \n");
   printMatrix(matrixA);
   printf("MATRIX B - \n");
   printMatrix(matrixB);
   printf("MATRIX RESULT - \n");
   printMatrix(matrixResult);
   */

   clock_gettime(CLOCK_MONOTONIC, &finish);
   elapsed = (finish.tv_sec - start.tv_sec);
   elapsed += (finish.tv_sec - start.tv_sec) / 1000000000.0;
   printf("TOTAL MULTI THREAD TIME SPENT: %f seconds \n", elapsed);
}
