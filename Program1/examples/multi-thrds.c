/**
 * A pthread program illustrating how to
 * create a simple thread and some of the pthread API
 * This program implements the summation function where
 * the summation operation is run as a separate thread.
 *
 * Most Unix/Linux/OS X users
 * gcc thrd.c -lpthread
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int sum; /* this data is shared by the thread(s) */

void *runner(void *param); /* the thread */

int main(int argc, char *argv[])
{
pthread_t tid[10]; /* the thread identifier */
pthread_attr_t attr; /* set of attributes for the thread */

/* get the default attributes */
pthread_attr_init(&attr);

/* create the thread */
for (int i = 0; i < 10; i++)
{
   int *x = (int *)malloc(sizeof(int));
   *x = i;
   pthread_create(&(tid[i]),&attr,runner, (void *) x);
   printf("Parent: Created thread with ID: %d\n", tid[i]);
}

/* now wait for the thread to exit */
for (int i =9; i>= 0; i--)
{
   pthread_join(tid[i],NULL);
   printf("Parent: Joined thread %d\n", tid[i]);
}

printf("Parent : Done\n");
}

/**
 * The thread will begin control in this function
 */
void *runner(void *param) 
{
int  q =  *((int *) param);
printf("Child %d : Thread created\n", q);
	pthread_exit(0);
}
