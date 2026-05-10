#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>
#include <mutex>
#include <mpi.h>
#include <vector>
#include <chrono>
#include <algorithm>
#include <vector>

static int rank, size;

class Task
{
private:
    static const int TOTAL_WEIGHT = 100;

public:
    int weight;
    Task() : weight(1){}
    Task(int tasks, int iteration) 
    {
        int topProc = iteration % size;
        int dist = std::abs(rank - topProc);
        dist = std::min(dist, size - dist);

        int max = size/2+1;
        int rawWeight = max - dist;
        
        
        int sum = max*(max + 1);
        max--;
        sum += max*(max + 1);
        sum/=2;
        if(size!=1)sum--;
        
        weight = (rawWeight * TOTAL_WEIGHT ) / sum;
    }

    void execute()
    {
        auto start = std::chrono::steady_clock::now();
        auto end = start + std::chrono::milliseconds(weight);
        volatile double i;
        while (std::chrono::steady_clock::now() < end)
        {
            i += std::sin(i) + std::cos(i);
        }
    }
};

class Dispatcher;

class Worker
{
private:
    std::vector<Task> _tasks;
    const int _tasksCount;
    int _tasksToDo = 0;

    Dispatcher *_dispatcher;
    std::mutex _sync;

public:
    Worker(int tasksCount);

    ~Worker();

    std::vector<int> stealTasks(int minCount)
    {
        _sync.lock();
        int toShare = _tasksToDo / 2;

        if (toShare < minCount)
        {
            _sync.unlock();
            return std::vector<int>(0);
        }

        std::vector<int> weights;
        weights.resize(toShare);

        _tasksToDo -= toShare;
        
        for (int i = 0; i < toShare; i++)
        {
            weights[i] = _tasks[_tasksToDo + i].weight;
        }
        
        _sync.unlock();
        return weights;
    }

    void setTasks(int *weights, int count)
    {
        _sync.lock();
        _tasks.resize(count);
        for (int i = 0; i < count; i++)
        {
            _tasks[i].weight = weights[i];
        }
        _tasksToDo = count;
        _sync.unlock();
    }

    void run(int iterations);
};

class Dispatcher
{
private:
    Worker *_worker;

    std::thread _listenerThread;

    static const int REQUEST_TAG = 1;
    static const int ANSWER_TAG = 2;
    static const int DATA_TAG = 3;
    static const int STOP_SIGNAL = -1;
    static const int MIN_TASKS_TO_SHARE = 5;

    void listen()
    {
        while (true)
        {
            int target;

            MPI_Recv(&target, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (target == STOP_SIGNAL)
                break;

            auto stolen = _worker->stealTasks(MIN_TASKS_TO_SHARE);
            int toShare = stolen.size();
            MPI_Send(&toShare, 1, MPI_INT, target, ANSWER_TAG, MPI_COMM_WORLD);

            if (toShare <= 0) continue;

            MPI_Send(stolen.data(), toShare, MPI_INT, target, DATA_TAG, MPI_COMM_WORLD);
        }
    }

public:
    Dispatcher(Worker *worker) : _worker(worker)
    {
        _listenerThread = std::thread(&Dispatcher::listen, this);
    }
    ~Dispatcher()
    {
        MPI_Send(&STOP_SIGNAL, 1, MPI_INT, rank, REQUEST_TAG, MPI_COMM_WORLD);
        if (_listenerThread.joinable())
            _listenerThread.join();
    }

    bool getTasks()
    {
        for (int i = 0; i < size; ++i)
        {
            if (i == rank)
                continue;

            int received = 0;
            MPI_Send(&rank, 1, MPI_INT, i, REQUEST_TAG, MPI_COMM_WORLD);
            MPI_Recv(&received, 1, MPI_INT, i, ANSWER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (received == 0)
                continue;

            int *weights = new int[received];
            MPI_Recv(weights, received, MPI_INT, i, DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            _worker->setTasks(weights, received);
            delete[] weights;
            return true;
        }
        return false;
    }
};
const int Dispatcher::STOP_SIGNAL;
Worker::Worker(int tasksCount) : _tasksCount(tasksCount)
{
    _tasks.resize(tasksCount);
    _dispatcher = new Dispatcher(this);
}

Worker::~Worker()
{
    delete _dispatcher;
}

void Worker::run(int iterations)
{
    auto start = MPI_Wtime();

    for (int i = 0; i < iterations; ++i)
    {
        _sync.lock();
        std::fill(_tasks.begin(), _tasks.end(), Task(_tasksCount, i));
        _tasksToDo = _tasksCount;
        _sync.unlock();

        while (true)
        {

            _sync.lock();
            if (_tasksToDo > 0)
            {
                _tasksToDo--;
                Task task = _tasks[_tasksToDo];
                _sync.unlock();
                task.execute();
            }
            else
            {
                _sync.unlock();
                if (!_dispatcher->getTasks())
                    break;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    double end = MPI_Wtime();
    std::cout << "Rank " << rank << ": completed in " << end - start << std::endl;
}

int main(int argc, char **argv)
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided != MPI_THREAD_MULTIPLE)
    {
        std::cerr << "MPI_THREAD_MULTIPLE not supported" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int ITERATIONS = 10;
    int TASKS = 10;
    {
        Worker worker(TASKS / size);
        worker.run(ITERATIONS);
    }

    MPI_Finalize();
    return 0;
}