#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

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

int N; /* Number of slots */
int T; /* Number of objects (ignored, left in for file compatibility) */
int p; /* Number of players */
int start = 0; /* Signal to start each game */
int end = 0; /* Signal to end the whole game */
int threads = 0; /* Number of threads running */
int *scores; /* Array of scores of the player in each slot */
char **names; /* Array of names of the player in each slot */
int *playerScores; /* Array of all player scores */
char **playerNames; /* Array of all player names */
struct Queue* numbers; /* Numbers queue */
pthread_mutex_t queueLock; /* Mutex for numbers queue */
struct Queue* players; /* Players queue */
pthread_mutex_t playerLock; /* Mutex for players queue */

/* Prints the passed  queue from last in to first in */
void printQueue(struct Queue *q) {
    struct Node* current = q->head;
    printf("\n(");
    while(current != NULL) {
        printf(" %d", current->x);
        current = current->next;
    }
    printf(" )\n\n");
}

/* Pushes an integer x to a new node at the beginning
|* of the passed queue. Calls printQueue() if second
|* argument is nonzero. */
void push(struct Queue *q, int x, int print) {
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
    }
    if(print)
        printQueue(q);
}

/* Returns the integer in the node at the end of the passed
|* queue. Calls printQueue() if second argument is not 0. */
int pop(struct Queue *q, int print) {
    struct Node* tail = q->tail;
    int result = tail->x;
    q->tail = tail->prev;
    if(q->size == 1) {
        q->head = NULL;
    } else {
        q->tail->next = NULL;
    }
    q->size -= 1;
    free(tail);
    if(print)
        printQueue(q);
    return result;
}

/* Function passed to pthreads */
void *runner(void *param);

/* Main method (dealer) */
int main(int argc, char *argv[])
{
    /* Preprocessing */
    FILE *input;
    input = fopen(argv[1], "r");
    fscanf(input, "%d", &N);
    fscanf(input, "%d", &T);
    fscanf(input, "%d\n", &p);

    playerNames = malloc(sizeof(char*) * p);
    playerScores = malloc(sizeof(int) * p);
    for(int i = 0; i < p; i++) {
        playerNames[i] = malloc(sizeof(char) * 20);
        fgets(playerNames[i], sizeof(playerNames[i]), input);
        playerNames[i][strcspn(playerNames[i], "\n")] = '\0';
        playerScores[i] = 0;
    }

    fclose(input);

    if(argc != 2) {
        printf("ERROR: Wrong number of arguments. Received %d, expecting 1.\n\n", argc - 1);
        return -1;
    }

    printf("Number of threads : %d\n", N);

    /* Initialize numbers queue */
    numbers = malloc(sizeof(struct Queue));
    numbers->size = 0;
    numbers->head = NULL;
    numbers->tail = NULL;

    /* Initialize players queue */
    players = malloc(sizeof(struct Queue));
    players->size = 0;
    players->head = NULL;
    players->tail = NULL;

    /* Initialize numbers queue lock */
    if (pthread_mutex_init(&queueLock, NULL) != 0) {
        printf("ERROR: Queue mutex initialization has failed\n");
        return -1;
    }

    /* Initialize players queue lock */
    if (pthread_mutex_init(&playerLock, NULL) != 0) {
        printf("ERROR: Player mutex initialization has failed\n");
        return -1;
    }
    
    /* Initialize scores and names arrays */
    scores = malloc(sizeof(int) * N);
    names = malloc(sizeof(char*) * p);
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
        names[i] = playerNames[i]; /* Assigns first N players to N slots */
        int *player = malloc(sizeof(int));
        *player = i;
        pthread_create(&(tid[i]), &attr, runner, (void *) player);
    }

    /* Pushes the remaining players to a queue */
    for(int i = N; i < p; i++) {
        push(players, i, 0);
    }

    /* Waits until all N threads are ready */
    while(threads < N);
    printf("\n-- All threads ready --\n\n");

    /* Signal the game to start */
    start = 1;

    /* Repeat until all threads have finished */
    srand(time(NULL));
    while(threads) {
        /* Generate a new number if queue has N or fewer numbers */
        pthread_mutex_lock(&queueLock); /* Begin critical section "deal" to push a number to the queue */
        if(numbers->size <= N) {
            int x = rand() % 40;
            printf("Dealer is pushing %d to the queue\n", x);
            push(numbers, x, 1);
        }
        pthread_mutex_unlock(&queueLock); /* End critical section "deal" after the number has been pushed */
    }

    /* Wait for all threads to finish */
    for(int i = 0; i < N; i++) {
        pthread_join(tid[i], NULL);
    }

    printf("\n------ Game Over ------\n\n");

    /* Print the final score for each player */
    for(int i = 0; i < p; i++) {
        printf("Final score for Player %s: %d\n", playerNames[i], playerScores[i]);
    }
}

/* Thread method (players). Param is the slot. */
void *runner(void *param) {
    /* Preprocessing */
    int k = *((int*) param); /* Slot */
    int currentPlayer = k;
    printf("Thread %d started\n", k);
    threads++;

    /* Wait for the game start signal */
    while(!start);

    /* Repeat until there are no players in the queue */
    while(!end) {
        while(scores[k] < 100 && !end) {
            pthread_mutex_lock(&queueLock); /* Begin critical section "grab" while game is still running */
            /* If the queue is not empty */
            if(numbers->size > 0) {
                /* Get the next number */
                int x = pop(numbers, 0);
                int score;
                int result;
                printf("Player %s (Slot %d) popped %d from the queue.\n", names[k], k, x);
                if(x <= N) {
                    score = x;
                    printf("\tScored. Player will score %d. Nothing will be pushed.\n", score);
                } else if((x % N == k) || (x % N == (k + 1) % N)) {
                    score = x * 2 / 5;
                    printf("\tMatched. Player will score %d. Need to push 2x/5 back on queue\n", score);
                } else {
                    printf("\tFailed. Player will not score. Need to push x - 2 back on queue\n");
                }
                printQueue(numbers);

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
                    pthread_mutex_lock(&queueLock); /* Begin critical section "matched" if a number needs to be pushed to the queue */
                    printf("Player %s (Slot %d) is pushing %d to the queue\n", names[k], k, x - score);
                    push(numbers, x - score, 1);
                    pthread_mutex_unlock(&queueLock); /* End critical section "matched" after the number has been pushed */
                } else {
                    pthread_mutex_lock(&queueLock); /* Begin critical section "fail" if a number needs to be pushed to the queue */
                    printf("Player %s (Slot %d) is pushing %d to the queue\n", names[k], k, x - 2);
                    push(numbers, x - 2, 1);
                    pthread_mutex_unlock(&queueLock); /* End critical section "fail" after the number has been pushed */
                }
            } else {
                pthread_mutex_unlock(&queueLock); /* End critical section "grab" if there is no number to take from the queue */
            }
        }
        pthread_mutex_lock(&playerLock); /* Begin critical section "swap" once player score reaches 100 */
        if(players->size == 0) { /* If the players queue is empty, leave the game. If the game has not been ended, end it */
            printf("Player %s is leaving with a score of %d\n\n", names[k], scores[k]);
            if(end == 0) {
                printf("Player %s has ended the game.\n\n", names[k]);
            }
            end = 1;
        } else { /* Otherwise, the current player will leave the game and a new player will swap in to that slot */
            int oldPlayer = currentPlayer;
            currentPlayer = pop(players, 0);
            playerScores[oldPlayer] = scores[k];
            names[k] = playerNames[currentPlayer];
            scores[k] = 0;
            printf("\nPlayer %s is leaving with a score of %d."
                   "\nPlayer %s is starting with a score of %d in slot %d.\n\n",
                   playerNames[oldPlayer], playerScores[oldPlayer], names[k], scores[k], k);
        }
        pthread_mutex_unlock(&playerLock); /* End critical section "swap" after leaving the game */
    }
    playerScores[currentPlayer] = scores[k]; /* Update the final score of the player after the game ends */
    threads--; /* Mark the thread as finished */
}