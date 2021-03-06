#include <iostream>
#include <fstream>
#include <queue>
#include <deque>
#include <chrono>
#include <thread>
#include <iterator>
#include <pthread.h>

using namespace std;

class Philosopher
{
public:
    Philosopher(int index, string name, deque<int> requests)
    {
        this->requests = requests;
        this->tools = requests.size();
        this->index = index;
        this->name = name;
    }
    int index;           /* Index of the philosopher for the banker's algorithm structures */
    int tools;           /* Number of tools needed */
    string name;         /* Name of the philosopher */
    deque<int> requests; /* List of tools needed */
};

int k; /* Number of tool types */
int m; /* Number of tables */
int n; /* Number of philosophers */

/* Structures for deadlock avoidance */
vector<int> available;
vector<vector<int>> maximum;
vector<vector<int>> allocation;

queue<Philosopher *> philosophers; /* Queue of philosophers waiting for a table */

string buffer;               /* Used to clear ifstream */
int start = 0;               /* Start game signal */
int threads = 0;             /* Number of threads running */
pthread_mutex_t queueLock;   /* Lock for the philosopher queue */
pthread_mutex_t outputLock;  /* Lock for printing to output */
pthread_mutex_t requestLock; /* Lock for requesting a tool */

/* Function passed to pthreads */
void *runner(void *param);

int main(int argc, char *argv[])
{
    /* Preprocessing */
    if (argc != 2)
    {
        printf("ERROR: Wrong number of arguments. Received %d, expecting 1.\n\n", argc - 1);
        return -1;
    }

    ifstream input;
    input.open(argv[1]);
    input >> k; // Number of tool types
    input >> m; // Number of tables
    input >> n; // Number of philosophers

    if (k < 1 || m < 1 || n < 1)
    {
        cout << "ERROR: Number read from file is < 1" << endl;
        return -1;
    }

    /* Reading in the starting tool counts as available */
    available = vector<int>(k, 0);
    for (int i = 0; i < k; i++)
    {
        input >> available[i];
        if (available[i] < 1)
        {
            printf("ERROR: Quantity for tool #%d read from file is < 1", i + 1);
            return -1;
        }
    }
    getline(input, buffer);

    /* Initializing maximum and allocation structures and creating philosopher objects */
    maximum = vector<vector<int>>(n, vector<int>(k, 0));
    allocation = vector<vector<int>>(n, vector<int>(k, 0));
    for (int i = 0; i < n; i++)
    {
        string name;
        getline(input, name, ' ');
        int p;
        input >> p;
        deque<int> requests;
        for (int j = 0; j < p; j++)
        {
            int x;
            input >> x;
            maximum[i][x]++;
            requests.push_back(x);
        }
        philosophers.push(new Philosopher(i, name, requests));
        getline(input, buffer);
    }

    /* Initialize queue lock */
    if (pthread_mutex_init(&queueLock, NULL) != 0)
    {
        printf("ERROR: Queue mutex initialization has failed\n");
        return -1;
    }

    /* Initialize output lock */
    if (pthread_mutex_init(&outputLock, NULL) != 0)
    {
        printf("ERROR: Output mutex initialization has failed\n");
        return -1;
    }

    /* Initialize request lock */
    if (pthread_mutex_init(&requestLock, NULL) != 0)
    {
        printf("ERROR: Request mutex initialization has failed\n");
        return -1;
    }

    /* Create all threads */
    pthread_t tid[m];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    printf("\n--- Running Threads ---\n\n");
    for (int i = 0; i < m; i++)
    {
        int *index = new int;
        *index = i;
        pthread_create(&(tid[i]), &attr, runner, (void *)index);
    }

    /* Wait for threads to all start */
    while (threads < m)
        ;
    printf("\n-- All threads ready --\n\n");

    /* Signal the game to start */
    start = 1;

    /* Wait for all threads to finish */
    for (int i = 0; i < m; i++)
    {
        pthread_join(tid[i], NULL);
    }

    cout << "All done" << endl;
}

/* Calculate need structure for banker's algorithm */
vector<vector<int>> calculateNeed(vector<vector<int>> &m, vector<vector<int>> &a)
{
    vector<vector<int>> need = vector<vector<int>>(n, vector<int>(k, 0));
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < k; j++)
        {
            need[i][j] = m[i][j] - a[i][j];
        }
    }

    return need;
}

// Returns true if arr1 is less than or equal to arr2 for every item
bool lteq(vector<int> &arr1, vector<int> &arr2)
{
    for (int i = 0; i < arr1.size(); i++)
    {
        if (arr1[i] > arr2[i])
            return false;
    }
    return true;
}

/* Check if the given request will cause the state to be unsafe */
bool safety(int index, vector<int> request, vector<int> available, vector<vector<int>> maximum, vector<vector<int>> allocation)
{
    vector<vector<int>> need = calculateNeed(maximum, allocation);
    for (int i = 0; i < k; i++)
    {
        available[i] -= request[i];
        allocation[index][i] += request[i];
        need[index][i] -= request[i];
    }

    bool finish[n] = {false};

    /* Find a philosopher which can finish with the available resources, and repeat until none are left */
    for (int i = 0; i < n; i++)
    {
        if (finish[i] == false && lteq(need[i], available))
        {
            for (int j = 0; j < k; j++)
            {
                available[j] = available[j] + allocation[i][j];
            }
            finish[i] = true;
            i = -1;
        }
    }

    /* If all philosophers cannot finish, the state is unsafe */
    for (int i = 0; i < n; i++)
    {
        if (!finish)
            return false;
    }

    /* Otherwise, the state is safe */
    return true;
}

/* Returns if a given request of tools is valid or not */
bool request(int index, vector<int> tools)
{
    vector<vector<int>> need = calculateNeed(maximum, allocation);
    vector<int> request(k, 0);
    for (int tool : tools)
    {
        request[tool]++;
    }

    /* Throws error if philosopher requests more than he would need */
    if (!lteq(request, need[index]))
        throw overflow_error("Exceeded maximum allowed");

    /* Returns false if there are not enough resources available */
    if (!lteq(request, available))
        return false;

    /* Returns true or false based on of the request will cause on unsafe state */
    bool result = safety(index, request, available, maximum, allocation);
    return result;
}

/* Thread method (philosophers). Param is the index of the philosopher */
void *runner(void *param)
{
    /* Preprocessing */
    int thread = *(int *)param;
    pthread_mutex_lock(&queueLock);
    cout << "Starting thread " << thread << endl;
    pthread_mutex_unlock(&queueLock);

    threads++;

    /* Wait for the game start signal */
    while (!start)
        ;

    /* While there are still philosophers waiting */
    Philosopher *p;
    while (start)
    {
        /* Grab a philosopher from the queue if there is one */
        pthread_mutex_lock(&queueLock);
        if (philosophers.size() > 0)
        {
            p = philosophers.front();
            philosophers.pop();
            pthread_mutex_unlock(&queueLock);
        }
        /* End the thread otherwise */
        else
        {
            pthread_mutex_unlock(&queueLock);
            break;
        }

        pthread_mutex_lock(&outputLock);
        cout << p->name << " sits down at table " << thread << endl;
        pthread_mutex_unlock(&outputLock);

        /* Aqcquire tools */
        deque<int> currentTools;

        while (currentTools.size() < p->tools)
        {
            auto t = time(0);
            bool result;
            vector<int> requestedTools;
            /* Get the tools to request */
            for (int i = 0; i <= t % 2 && i < p->requests.size(); i++)
            {
                int tool = p->requests.front();
                p->requests.pop_front();
                requestedTools.push_back(tool);
            }

            /* Request the tools */
            pthread_mutex_lock(&requestLock);
            pthread_mutex_lock(&outputLock);
            cout << p->name << " requests";
            for (auto tool : requestedTools)
            {
                cout << " " << tool;
            }
            bool granted = request(p->index, requestedTools);
            cout << ", " << (granted ? "granted" : "denied") << endl;
            pthread_mutex_unlock(&outputLock);
            if (granted)
            {
                /* Adjust the allocation of tools if the request is granted */
                for (auto tool : requestedTools)
                {
                    currentTools.push_back(tool);
                    allocation[p->index][tool]++;
                    available[tool]--;
                }
            }
            else
            {
                /* Otherwise add those tools back to the list of requests */
                for (auto tool : requestedTools)
                {
                    p->requests.push_back(tool);
                }
            }
            pthread_mutex_unlock(&requestLock);

            /* Sleep between requests */
            this_thread::sleep_for(chrono::seconds(2) + chrono::milliseconds(t % 1000));
        }

        /* Eat */
        this_thread::sleep_for(chrono::seconds(2));

        /* Return tools */
        pthread_mutex_lock(&requestLock);
        pthread_mutex_lock(&outputLock);
        cout << p->name << " finishes, releasing ";
        for (auto tool : currentTools)
        {
            cout << tool << " ";
            allocation[p->index][tool]--;
            available[tool]++;
        }
        cout << endl;
        pthread_mutex_unlock(&outputLock);
        pthread_mutex_unlock(&requestLock);
    }

    threads--;
}