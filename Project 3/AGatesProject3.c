#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define NRMAXPHASES 5
#define NRCPUTHREADS 8
#define NRJOBTHREADS 4
#define NRIOTHREADS 4
#define JOBCREATERUNTIME 3
#define QUEUESIZE 100

FILE *fp;

pthread_mutex_t jobCreateMutex;
pthread_mutex_t cpuMutex;
pthread_mutex_t ioMutex;
pthread_mutex_t finishedMutex;

int cpuCompleted = 0;
int ioCompleted = 0;
int finishedCompleted = 0;
int cpuRunning = 0;
int ioRunning = 0;
int currentJobID = 0;

typedef struct job
{
   int jobID;
   int nrPhases;
   int curPhase;
   //Phase types: (X % 2 = 0) = CPU, (X % 2 = 1) = IO
   int duration[NRMAXPHASES];
   int isComplete;
}job;

typedef struct Queue
{
   int capacity;
   int size;
   int front;
   int rear;
   job *jobs;
}Queue;

Queue *readRunQueue;
Queue *ioRunQueue;
Queue *finishedQueue;

job createJob(int id, int phaseCount)
{
   job newJob;
   newJob.jobID = id;
   newJob.nrPhases = phaseCount;
   newJob.curPhase = 0;
   return newJob;
}

Queue * createQueue(int maxElements)
{
   Queue *Q;
   Q = (Queue *)malloc(sizeof(Queue));

   Q->jobs = (job *)malloc(sizeof(job)*maxElements);
   Q->size = 0;
   Q->capacity = maxElements;
   Q->front = 0;
   Q->rear = -1;

   return Q;
}

job dequeue(Queue *Q)
{
   job deQueuedJob = Q->jobs[Q->front];
   Q->size--;
   Q->front++;
   return deQueuedJob;
}

void enqueue(Queue *Q,job curJob)
{
   Q->size++;
   Q->rear = Q->rear + 1;
   Q->jobs[Q->rear] = curJob;
}

void printQueue(Queue *Q)
{
   if(Q->size == 0)
   {
      fprintf(fp, "QUEUE EMPTY \n");
   }
   else
   {
      int i;
      printf("TOP ");

      for(i = 0; i < Q->size; i++)
      {
         if(i+1 == Q->size)
         {
            fprintf(fp, "%d BOTTOM \n", Q->jobs[Q->front + i]);
         }
         else
         {
            fprintf(fp, "%d -> ", Q->jobs[Q->front + i]);
         }
      }
   }
}

static void * cpuExecutionThreads()
{
   while(readRunQueue->size > 0 || ioRunQueue->size > 0 || finishedCompleted != 4)
   {
      pthread_mutex_lock(&cpuMutex);

      if(readRunQueue->size > 0)
      {
         job curJob = dequeue(readRunQueue);
         pthread_mutex_unlock(&cpuMutex);

         fprintf(fp, "*CPU RUNNING JOB - %d, WAITING FOR - %d SECONDS, CUR PHASE - %d \n", curJob.jobID, curJob.duration[curJob.curPhase], curJob.curPhase);
         cpuRunning++;
         sleep(curJob.duration[curJob.curPhase]);

         if(curJob.curPhase == curJob.nrPhases - 1)
         {
            fprintf(fp, "%JOB - %d TO FINISHED FROM CPU\n", curJob.jobID);
            curJob.isComplete = 1;
            pthread_mutex_lock(&finishedMutex);
            enqueue(finishedQueue, curJob);
            pthread_mutex_unlock(&finishedMutex);
         }
         else
         {
            fprintf(fp, "%JOB - %d TO FINISHED FROM CPU\n", curJob.jobID);
            curJob.curPhase++;
            pthread_mutex_lock(&ioMutex);
            enqueue(ioRunQueue, curJob);
            pthread_mutex_unlock(&ioMutex);
         }
         sleep(1);
         cpuRunning--;
      }
      else
      {
         pthread_mutex_unlock(&cpuMutex);
      }
   }
   cpuCompleted++;
}

static void * ioExecutionThreads()
{
   while(readRunQueue->size > 0 || ioRunQueue->size > 0 || finishedCompleted != 4)
   {
      pthread_mutex_lock(&ioMutex);
      if(ioRunQueue->size > 0)
      {
         job curJob = dequeue(ioRunQueue);
         pthread_mutex_unlock(&ioMutex);

         fprintf(fp, "**IO RUNNING JOB - %d, WAITING FOR %d SECONDS, CUR PHASE - %d \n", curJob.jobID, curJob.duration[curJob.curPhase], curJob.curPhase);
         ioRunning++;
         sleep(curJob.duration[curJob.curPhase]);

         if(curJob.curPhase == curJob.nrPhases - 1)
         {
            fprintf(fp, "%JOB - %d TO FINISHED FROM IO\n", curJob.jobID);
            curJob.isComplete = 1;
            pthread_mutex_lock(&finishedMutex);
            enqueue(finishedQueue, curJob);
            pthread_mutex_unlock(&finishedMutex);
         }
         else
         {
            fprintf(fp, "%%%JOB - %d TO CPU FROM IO\n", curJob.jobID);
            curJob.curPhase++;
            pthread_mutex_lock(&cpuMutex);
            enqueue(readRunQueue, curJob);
            pthread_mutex_unlock(&cpuMutex);
         }
         sleep(1);
         ioRunning--;
      }
      else
      {
         pthread_mutex_unlock(&ioMutex);
      }
   }
   ioCompleted++;
}

static void * jobCreationThreads()
{
   int i = 0, j = 0;
   while(i < JOBCREATERUNTIME)
   {
      pthread_mutex_lock(&finishedMutex);
      if(finishedQueue->size > 0)
      {
         job curJob = dequeue(finishedQueue);
         fprintf(fp, "***JOB - %d FINISHED\n", curJob.jobID);
      }
      pthread_mutex_unlock(&finishedMutex);

      pthread_mutex_lock(&jobCreateMutex);
      job newJob;
      currentJobID++;
      newJob.jobID = currentJobID;
      newJob.curPhase = 0;
      newJob.nrPhases = NRMAXPHASES - (rand() % 5);
      for(j = 0; j < newJob.nrPhases; j++)
      {
         newJob.duration[j] = (rand() % 10) + 1;
      }
      newJob.isComplete = 0;
      pthread_mutex_unlock(&jobCreateMutex);

      fprintf(fp, "CREATING JOB - %d, WITH DURATION - %d SECONDS, NUMBER OF PHASES - %d\n", newJob.jobID,newJob.duration[0],newJob.nrPhases);

      pthread_mutex_lock(&cpuMutex);
      enqueue(readRunQueue, newJob);
      pthread_mutex_unlock(&cpuMutex);

      i++;
      sleep(3);
   }

   while(readRunQueue->size > 0 || ioRunQueue->size > 0 || finishedQueue->size > 0 || cpuCompleted != 8 || ioCompleted != 4)
   {
      pthread_mutex_lock(&finishedMutex);
      if(finishedQueue->size > 0)
      {
         job curJob = dequeue(finishedQueue);
         fprintf(fp, "***JOB - %d FINISHED\n", curJob.jobID);
      }
      pthread_mutex_unlock(&finishedMutex);

      if(cpuRunning == 0 && ioRunning == 0)
      {
         finishedCompleted++;
         break;
      }
   }
}

int main()
{
   fp = fopen("AGatesProject3Output.txt", "w");
   srand(time(NULL));
   int i;

   readRunQueue = createQueue(QUEUESIZE);
   ioRunQueue = createQueue(QUEUESIZE);
   finishedQueue = createQueue(QUEUESIZE);

   pthread_mutex_init(&jobCreateMutex,NULL);
   pthread_mutex_init(&cpuMutex,NULL);
   pthread_mutex_init(&ioMutex,NULL);
   pthread_mutex_init(&finishedMutex,NULL);
   
   pthread_t jobThreads[NRJOBTHREADS];
   pthread_t cpuThreads[NRCPUTHREADS];
   pthread_t ioThreads[NRIOTHREADS];

   for(i = 0; i < NRCPUTHREADS; i++)
   {
      if(i > 3)
      {
         pthread_create(&cpuThreads[i], NULL, cpuExecutionThreads, (void *)i);
      }
      else
      {
         pthread_create(&jobThreads[i], NULL, cpuExecutionThreads, (void *)i);
         pthread_create(&cpuThreads[i], NULL, jobCreationThreads, (void *)i);
         pthread_create(&ioThreads[i], NULL, ioExecutionThreads, (void *)i);
      }
   }

   for(i = 0; i < NRCPUTHREADS; i++)
   {
      if(i > 3)
      {
         pthread_join(cpuThreads[i], NULL);
      }
      else
      {
         pthread_join(jobThreads[i], NULL);
         pthread_join(cpuThreads[i], NULL);
         pthread_join(ioThreads[i], NULL);
      }
   }

   fprintf(fp, "Read/Run Queue - ");
   printQueue(readRunQueue);
   fprintf(fp, "I/O Queue - ");
   printQueue(ioRunQueue);
   fprintf(fp, "Finished Queue - ");
   printQueue(finishedQueue);

   //free(readRunQueue);
   //free(ioRunQueue);
   //free(finishedQueue);

   pthread_mutex_destroy(&jobCreateMutex);
   pthread_mutex_destroy(&cpuMutex);
   pthread_mutex_destroy(&ioMutex);
   pthread_mutex_destroy(&finishedMutex);
   pthread_exit(NULL);

   fclose(fp);
}
