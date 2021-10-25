#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *runner(void *param);

/* These variables need to be shared between the parent process
** and all threads, therefore they are declared globally */
int *P; /* Boolean array of whether a number is prime or not */
int n;  /* Upper limit for program to check for primes       */
int t;  /* How many threads the program should create        */
int x;  /* Count of numbers to check for each thread         */

int main(int argc, char *argv[])
{
    printf("This program will find all non-prime numbers between 2 and n using t threads\n");

    char str[255]; /* String to clear input buffer */
    int check;     /* Check for scanf return value */

    /* Checking input for n */
    while(1) {
        printf("Enter an integer n: ");
        check = scanf("%d", &n);
        if(check < 1) {
            scanf("%s", str);
            printf("n must be an integer\n");
        } else if(n < 1) {
            printf("n must be greater than 0\n");
        } else {
            break;
        }
    }

    /* Checking input for t */
    while(1) {
        printf("Enter an integer t: ");
        check = scanf("%d", &t);
        if(check < 1) {
            scanf("%s", str);
            printf("t must be an integer\n");
        } else if(t < 1) {
            printf("t must be greater than 0\n");
        } else {
            break;
        }
    }

    /* Initializing P to be size n of all true */
    P = (int *) malloc(sizeof(int) * n);
    
    for(int i = 0; i < n; i++) {
        P[i] = 1;
    }

    /*  */
    pthread_t tid[t];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    /* Integer division would provide the floor of n / t, but in some cases
    ** this would not leave enough room in each thread for all n numbers.
    ** This operation provides the ceiling of n / t instead, which will
    ** always be sufficient for n numbers divided among t threads. */
    x = (n + t - 1) / t;

    /* Creating t threads to run the runner function with a unique 'indexes' array as the parameter */
    for(int i = 0; i < t; i++)
    {
        int *indexes = (int *) malloc(sizeof(int) * x); /* This array of indexes will be checked by thread i */
        for(int j = 0; j < x; j++) {
            /* The threads are given numbers that are t indexes apart. This method makes sure that all
            ** threads have a relatively similar number of indexes to check, in contrast to assigning
            ** sequential indexes to each thread, which would leave all threads at a maximum size with
            ** at worst only a single index for the last thread to check ((i * t) + j) */
            indexes[j] = (j * t) + i;
        }

        /* Casting indexes to type (void *) to pass it to the created thread */
        pthread_create(&(tid[i]), &attr, runner, (void *) indexes);
    }

    /* Waiting for all t threads to finish */
    for(int i = 0; i < t; i++) {
        pthread_join(tid[i], NULL);
    }

    /* Outputting all prime numbers from 2 to n */
    printf("\nThe prime numbers from 2 to %d are:\n", n);
    for(int i = 1; i < n; i++) {
        if(P[i]) {
            printf("%d\n", i+1);
        }
    }

    /* Freeing allocated memory */
    free(P);

    return 0;
}

void *runner(void *param) {
    /* Casting the param back to its original type of (int *) */
    int *indexes = ((int *) param);

    for(int i = 0; i < x; i++) {
        int index = indexes[i];

        /* Checking that the index is greater than 0 and less than n,
        ** and therefore that the number represented is from 2 to n,
        ** and then checking whether that value of P is true */
        if(index > 0 && index < n && P[index]) {
            /* The array is zero indexed, but the first index represents the number one.
            ** When calculating multiples we need to adjust our index by 1 to account
            ** for this (the number 4 has an index of 3, 6 has an index of 5, etc). */
            int num = index + 1;

            /* Checking multiples of num from 2 to n / num, which is the maxiumum
            ** multiple possible greater than or equal to n */
            for(int j = 2; j <= n / num; j++) {
                P[(num * j) - 1] = 0; /* Un-adjusting by 1 to be able to index the array */
            }
        }
    }

    /* Freeing allocated memory (parent process has lost reference, must be freed here) */
    free(indexes);
}