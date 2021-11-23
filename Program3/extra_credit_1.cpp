#include <iostream>
#include <fstream>
#include <queue>
#include <deque>
#include <chrono>
#include <thread>
#include <iterator>
#include <pthread.h>

// #include <pthread.h>
// #include <semaphore.h>
// #include <stdlib.h>
// #include <time.h>
// #include <string.h>

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
    int index;
    int tools;
    string name;
    deque<int> requests;
};

int k; // Number of tool types
int m; // Number of tables
int n; // Number of philosophers

vector<int> available;
vector<vector<int>> maximum;
vector<vector<int>> allocation;

queue<Philosopher *> philosophers;

string buffer;
int start = 0;
int threads = 0;
pthread_mutex_t queueLock;
pthread_mutex_t outputLock;
pthread_mutex_t threadsLock;
pthread_mutex_t requestLock;

/* Function passed to pthreads */
void *runner(void *param);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("ERROR: Wrong number of arguments. Received %d, expecting 1.\n\n", argc - 1);
        return -1;
    }

    /* Preprocessing */
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

    /* Initialize threads lock */
    if (pthread_mutex_init(&threadsLock, NULL) != 0)
    {
        printf("ERROR: Threads mutex initialization has failed\n");
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
    for (int i = 0; i < n; i++)
    {
        if (!finish)
            return false;
    }
    return true;
}

bool request(int index, vector<int> tools)
{
    vector<vector<int>> need = calculateNeed(maximum, allocation);
    vector<int> request(k, 0);
    for (int tool : tools)
    {
        request[tool]++;
    }

    if (!lteq(request, need[index]))
        throw overflow_error("Exceeded maximum allowed");
    if (!lteq(request, available))
        return false;

    bool result = safety(index, request, available, maximum, allocation);
    return result;
}

/* Thread method (players). Param is the slot. */
void *runner(void *param)
{
    int thread = *(int *)param;
    pthread_mutex_lock(&queueLock);
    cout << "Starting thread " << thread << endl;
    pthread_mutex_unlock(&queueLock);

    pthread_mutex_lock(&threadsLock);
    threads++;
    pthread_mutex_unlock(&threadsLock);

    /* Wait for the game start signal */
    while (!start)
        ;

    Philosopher *p;
    while (start)
    {
        pthread_mutex_lock(&queueLock);
        if (philosophers.size() > 0)
        {
            p = philosophers.front();
            philosophers.pop();
            pthread_mutex_unlock(&queueLock);
        }
        else
        {
            pthread_mutex_unlock(&queueLock);
            break;
        }

        pthread_mutex_lock(&outputLock);
        cout << p->name << " sits down at table " << thread << endl;
        pthread_mutex_unlock(&outputLock);

        // Aqcquire tools
        deque<int> currentTools;

        while (currentTools.size() < p->tools)
        {
            auto t = time(0);
            bool result;
            vector<int> requestedTools;
            int numTools = t % 2 ? 1 : 2 + ((t / 25) % (p->requests.size()));
            for (int i = 0; i < numTools && i < p->requests.size(); i++)
            {
                int tool = p->requests.front();
                p->requests.pop_front();
                requestedTools.push_back(tool);
            }

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
                for (auto tool : requestedTools)
                {
                    currentTools.push_back(tool);
                    allocation[p->index][tool]++;
                    available[tool]--;
                }
            }
            else
            {
                for (auto tool : requestedTools)
                {
                    p->requests.push_back(tool);
                }
            }
            pthread_mutex_unlock(&requestLock);

            this_thread::sleep_for(chrono::seconds(2) + chrono::milliseconds(t % 1000));
        }

        // Eat
        this_thread::sleep_for(chrono::seconds(2));

        // Return tools
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
