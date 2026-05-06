#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>
#include <mutex>
#include <mpi.h>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

static int rank, size;

class Task
{
private:
    static const int MAX_WEIGHT = 1000;
    static const int MIN_WEIGHT = 10;

    int getRandomInt(int min, int max)
    {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

public:
    int weight;

    Task() : weight(getRandomInt(MIN_WEIGHT, MAX_WEIGHT)) {}

    void execute()
    {
        auto start = std::chrono::steady_clock::now();
        auto end = start + std::chrono::milliseconds(weight);
        while (std::chrono::steady_clock::now() < end)
        {
            volatile int i = getRandomInt(0, 1);
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

public:
    std::mutex _sync;
    std::atomic<bool> isRunning{true};

    Worker(int tasksCount);

    ~Worker();

    int tasksLeft()
    {
        return _tasksToDo;
    }

    int *stealTasks(int count)
    {
        int *weights = new int[count];
        _tasksToDo -= count;

        for (int i = 0; i < count; i++)
        {
            weights[i] = _tasks[_tasksToDo + i].weight;
        }

        return weights;
    }

    void setTasks(int *weights, int count)
    {
        _tasks.resize(count);
        for (int i = 0; i < count; i++)
        {
            _tasks[i].weight = weights[i];
        }
        _tasksToDo = count;
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
        while (_worker->isRunning)
        {
            int target;

            MPI_Recv(&target, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (target == STOP_SIGNAL)
                break;

            _worker->_sync.lock();
            int toShare = _worker->tasksLeft() / 2;

            if (toShare < MIN_TASKS_TO_SHARE)
            {
                _worker->_sync.unlock();
                toShare = 0;
                MPI_Send(&toShare, 1, MPI_INT, target, ANSWER_TAG, MPI_COMM_WORLD);
                continue;
            }

            int *stolen = _worker->stealTasks(toShare);
            _worker->_sync.unlock();

            MPI_Send(&toShare, 1, MPI_INT, target, ANSWER_TAG, MPI_COMM_WORLD);
            MPI_Send(stolen, toShare, MPI_INT, target, DATA_TAG, MPI_COMM_WORLD);
            delete[] stolen;
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

            _worker->_sync.lock();
            _worker->setTasks(weights, received);
            _worker->_sync.unlock();
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
    isRunning = false;
    delete _dispatcher;
}

void Worker::run(int iterations)
{
    auto start = MPI_Wtime();

    for (int i = 0; i < iterations; ++i)
    {
        _sync.lock();
        std::fill(_tasks.begin(), _tasks.end(), Task());
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
    int TASKS = 100;

    {
        Worker worker(TASKS / size);
        worker.run(ITERATIONS);
    }

    MPI_Finalize();
    return 0;
}