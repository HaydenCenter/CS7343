#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Implementing queue as linked list */
struct Node {
    int x;
    struct Node* next;
    struct Node* prev;
};
struct Queue {
    int size;
    struct Node* head;
    struct Node* tail;
};

int N; /* Number of players */
int T; /* Number of objects */
int threads = 0; /* Number of threads running */
int *scores; /* Array of scores for each thread */
struct Queue* q; /* FIFO queue */
pthread_mutex_t queueLock; /* Mutex for FIFO queue */


int game = 0; /* Count of players still playing the game
              |* (including dealer & queue). Ensures that
              |* the game does not end until the dealer
              |* has stopped generating numbers and all
              |* players have emptied their hands. */
pthread_mutex_t gameLock; /* Mutex for player count */

/* Prints the FIFO queue from last in to first in */
void printQueue() {
    struct Node* current = q->head;
    printf("\n(");
    while(current != NULL) {
        printf(" %d", current->x);
        current = current->next;
    }
    printf(" )\n\n");
}

/* Pushes an integer x to a new node at the beginning
|* of the queue. Calls printQueue() if second argument
|* is nonzero. */
void push(int x, int print) {
    struct Node* next = q->head;
    struct Node* head = malloc(sizeof(struct Node));
    head->x = x;
    head->next = next;
    head->prev = NULL;
    if(next != NULL)
        next->prev = head;
    q->head = head;
    q->size += 1;
    if(q->size == 1) {
        q->tail = q->head;
        /* Critical section "push" is necessary to keep the threads from thinking the
        |* is finished until the queue is empty, the dealer is done generating,
        |* and the threads are not holding a popped number. This section incrememnts
        |* the game counter if the queue was empty and gets pushed to. */
        pthread_mutex_lock(&gameLock); /* Begin critical section "push" */
        game++;
        pthread_mutex_unlock(&gameLock); /* End critical section "push" */
    }
    if(print)
        printQueue();
}

/* Returns the integer in the node at the end of the queue.
|* Calls printQueue() if second argument is nonzero. */
int pop(int print) {
    struct Node* tail = q->tail;
    int result = tail->x;
    q->tail = tail->prev;
    if(q->size == 1) {
        q->head = NULL;
        /* Critical section "pop" is necessary to keep the threads from thinking the
        |* is finished until the queue is empty, the dealer is done generating,
        |* and the threads are not holding a popped number. This section decrements
        |* the game counter if the queue will be empty after getting popped. */
        pthread_mutex_lock(&gameLock); /* Begin critical section "pop" */
        game--;
        pthread_mutex_unlock(&gameLock);  /* End critical section "pop" */
    } else {
        q->tail->next = NULL;
    }
    q->size -= 1;
    free(tail);
    if(print)
        printQueue();
    return result;
}

/* Function passed to pthreads */
void *runner(void *param);

/* Main method (dealer) */
int main(int argc, char *argv[])
{
    /* Preprocessing */
    if(argc != 3) {
        printf("ERROR: Wrong number of arguments. Received %d, expecting 2.\n\n", argc - 1);
        return -1;
    }

    N = atoi(argv[1]);
    T = atoi(argv[2]);

    if(N <= 0 || T <= 0) {
        printf("ERROR: Arguments must be positive integers. Received %s and %s.\n\n", argv[1], argv[2]);
        return -1;
    }

    printf("Number of threads : %d | Number of objects : %d\n", N, T);

    /* Initialize queue */
    q = malloc(sizeof(struct Queue));
    q->size = 0;
    q->head = NULL;
    q->tail = NULL;

    /* Initialize queue lock */
    if (pthread_mutex_init(&queueLock, NULL) != 0) {
        printf("ERROR: Queue mutex initialization has failed\n");
        return -1;
    }

    /* Initialize game lock */
    if (pthread_mutex_init(&gameLock, NULL) != 0) {
        printf("ERROR: Game mutex initialization has failed\n");
        return -1;
    }
    
    /* Initialize scores array */
    scores = malloc(sizeof(int) * N);
    for(int i = 0; i < N; i++) {
        scores[i] = 0;
    }

    /* Create all threads */
    pthread_t tid[N];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    printf("\n--- Running Threads ---\n\n");
    for(int i = 0; i < N; i++)
    {
        int *player = malloc(sizeof(int));
        *player = i;
        pthread_create(&(tid[i]), &attr, runner, (void *) player);
    }

    /* Waits until all N threads are ready */
    while(threads < N);
    printf("\n-- All threads ready --\n\n");

    /* Signal the game to start */
    pthread_mutex_lock(&gameLock); /* Begin critical section "generating" after creating all threads */
    game++;
    pthread_mutex_unlock(&gameLock); /* End critical section "generating" after making sure the game will continue */
        
    /* Repeat until T numbers generated */
    srand(time(NULL));
    int numbers = T;
    while(numbers) {
        /* Generate a new number if queue has N or fewer numbers */
        pthread_mutex_lock(&queueLock); /* Begin critical section "deal" to push a number to the queue */
        if(q->size <= N) {
            int x = rand() % 40;
            printf("Dealer is pushing %d to the queue\n", x);
            push(x, 1);
            numbers--;
        }
        pthread_mutex_unlock(&queueLock); /* End critical section "deal" after the number has been pushed */
    }

    printf("Done generating\n");

    pthread_mutex_lock(&gameLock); /* Begin critical section "done" after generating all T numbers */
    game--;
    pthread_mutex_unlock(&gameLock); /* End critical section "done" after ending the main process's need for the game to be running */

    /* Wait for all threads to finish */
    for(int i = 0; i < N; i++) {
        pthread_join(tid[i], NULL);
    }

    /* Print the final score for each player */
    for(int i = 0; i < N; i++) {
        printf("Final score for Thread %d : %d\n", i, scores[i]);
    }
}

/* Thread method (players). Param is the player index. */
void *runner(void *param) {
    /* Preprocessing */
    int k = *((int*) param);
    printf("Thread %d started\n", k);
    threads++;

    /* Wait for the game start signal */
    while(!game);

    /* Repeat until the game ends */
    while(game) {
        pthread_mutex_lock(&queueLock); /* Begin critical section "grab" while game is still running */
        /* If the queue is not empty */
        if(q->size > 0) {
            /* Get the next number */
            pthread_mutex_lock(&gameLock); /* Begin critical section "hold" if there are numbers in the queue */
            game++;
            pthread_mutex_unlock(&gameLock); /* End critical section "hold" after making sure the game will continue */
            int x = pop(0);
            int score;
            int result;
            printf("Thread %d popped %d from the queue.\n", k, x);
            if(x <= N) {
                score = x;
                printf("\tScored. Thread %d will score %d. Nothing will be pushed.\n", k, score);
            } else if((x % N == k) || (x % N == (k + 1) % N)) {
                score = x * 2 / 5;
                printf("\tMatched. Thread %d will score %d. Need to push 2x/5 back on queue\n", k, score);
            } else {
                printf("\tFailed. Player will not score. Need to push x - 2 back on queue\n");
            }
            printQueue();

            /* Sleep for 1 + x ms */
            pthread_mutex_unlock(&queueLock); /* End critical section "grab" after a number has been taken from the queue and saved */
            struct timespec req, rem;
            req.tv_sec = 0;
            req.tv_nsec = x * 1000000 + 1;
            nanosleep(&req, &rem);

            /* Process scoring */
            if(x <= N) {
                scores[k] += score;
            } else if((x % N == k) || (x % N == (k + 1) % N)) {
                scores[k] += score;
                pthread_mutex_lock(&queueLock); /* Begin critical section "matched"  if a number needs to be pushed to the queue */
                printf("Thread %d is pushing %d to the queue\n", k, x - score);
                push(x - score, 1);
                pthread_mutex_unlock(&queueLock); /* End critical section "matched"  after the number has been pushed */
            } else {
                pthread_mutex_lock(&queueLock); /* Begin critical section "fail" if a number needs to be pushed to the queue */
                printf("Thread %d is pushing %d to the queue\n", k, x - 2);
                push(x - 2, 1);
                pthread_mutex_unlock(&queueLock); /* End critical section "fail" after the number has been pushed */
            }
            pthread_mutex_lock(&gameLock); /* Begin critical section "release" after dealing with the object */
            game--;
            pthread_mutex_unlock(&gameLock); /* End critical section "release" after ending the threads need for the game to be running */
        } else {
            pthread_mutex_unlock(&queueLock); /* End critical section "grab" if there is no number to take from the queue */
        }
    }
}