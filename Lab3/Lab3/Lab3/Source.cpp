#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <random>
#include <vector>
#include <atomic>

using namespace std;

class ThreadPool {
private:
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool accepting = false;
    bool stop = false;
    int workerCount;

    atomic<int> createdThreads;
    atomic<long long> totalWaitTimeMicroseconds;
    atomic<int> waitCount;
    atomic<long long> totalTaskExecTimeMicroseconds;
    atomic<int> taskCount;
    atomic<long long> totalQueueLength;
    atomic<int> queueLengthCount;

    atomic<bool> paused;
    mutex mtx_pause;
    condition_variable cv_pause;

public:
    ThreadPool(int count) : workerCount(count),
        createdThreads(0),
        totalWaitTimeMicroseconds(0),
        waitCount(0),
        totalTaskExecTimeMicroseconds(0),
        taskCount(0),
        totalQueueLength(0),
        queueLengthCount(0),
        paused(false)
    {
        cout << "[POOL] Thread pool created with " << workerCount << " worker threads.\n";
    }

    bool isEmpty() {
        lock_guard<mutex> lock(mtx);
        return tasks.empty();
    }

    void addTask(function<void()> task) {
        lock_guard<mutex> lock(mtx);
        if (!accepting) {
            return;
        }
        tasks.push(task);
        int q_size = tasks.size();
        totalQueueLength += q_size;
        queueLengthCount++;
        cout << "[POOL] Task added to queue. Current queue size: " << q_size << "\n";
        cv.notify_all();
    }

    void workerFunction(int id) {
        thread_local bool counted = false;
        if (!counted) {
            createdThreads++;
            counted = true;
        }

        while (true) {
            function<void()> task;
            {
                unique_lock<mutex> lock(mtx);
                auto wait_start = chrono::steady_clock::now();
                cv.wait(lock, [this] { return stop || (!tasks.empty() && !accepting); });
                auto wait_end = chrono::steady_clock::now();
                long long wait_micro = chrono::duration_cast<chrono::microseconds>(wait_end - wait_start).count();
                totalWaitTimeMicroseconds += wait_micro;
                waitCount++;

                if (stop) {
                    cout << "[WORKER " << id << "] Stopping thread.\n";
                    break;
                }

                if (!tasks.empty()) {
                    task = tasks.front();
                    tasks.pop();
                    cout << "[WORKER " << id << "] Task taken. Remaining tasks: " << tasks.size() << "\n";
                }
            }

            if (task) {
                {
                    unique_lock<mutex> pause_lock(mtx_pause);
                    cv_pause.wait(pause_lock, [this] { return !paused.load(); });
                }

                cout << "[WORKER " << id << "] Executing task...\n";
                auto exec_start = chrono::steady_clock::now();
                task();
                auto exec_end = chrono::steady_clock::now();
                long long exec_micro = chrono::duration_cast<chrono::microseconds>(exec_end - exec_start).count();
                totalTaskExecTimeMicroseconds += exec_micro;
                taskCount++;
                cout << "[WORKER " << id << "] Task execution finished.\n";
            }
        }
    }

    void startAcceptingTasks() {
        lock_guard<mutex> lock(mtx);
        accepting = true;
        cout << "[POOL] Now accepting new tasks.\n";
    }

    void stopAcceptingAndExecute() {
        {
            lock_guard<mutex> lock(mtx);
            accepting = false;
        }
        cout << "[POOL] 30 seconds elapsed. Stopping acceptance of new tasks and starting execution.\n";
        cv.notify_all();

        double avgWaitSec = (waitCount > 0) ? (totalWaitTimeMicroseconds.load() / static_cast<double>(waitCount)) / 1e6 : 0.0;
        double avgTaskExecSec = (taskCount > 0) ? (totalTaskExecTimeMicroseconds.load() / static_cast<double>(taskCount)) / 1e6 : 0.0;
        double avgQueueLength = (queueLengthCount > 0) ? totalQueueLength.load() / static_cast<double>(queueLengthCount) : 0.0;

        cout << "[STATISTICS] Created threads: " << createdThreads.load() << "\n";
        cout << "[STATISTICS] Average waiting time per thread: " << avgWaitSec << " seconds\n";
        cout << "[STATISTICS] Average task execution time: " << avgTaskExecSec << " seconds\n";
        cout << "[STATISTICS] Average queue length: " << avgQueueLength << "\n";
    }

    void stopWorkers() {
        {
            lock_guard<mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
    }

    void setPause(bool p) {
        paused = p;
        if (!p) {
            unique_lock<mutex> lock(mtx_pause);
            cv_pause.notify_all();
        }
    }
};

int main() {
    ThreadPool pool(2);
    vector<thread> workers;
    for (int i = 0; i < 2; i++) {
        workers.push_back(thread(&ThreadPool::workerFunction, &pool, i));
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(2, 8);

    thread generator([&pool, &gen, &dist]() {
        cout << "[GENERATOR] Task generator started.\n";
        while (true) {
            int taskTime = dist(gen);
            function<void()> task = [taskTime]() {
                cout << "[TASK] Task executing for " << taskTime << " seconds.\n";
                this_thread::sleep_for(chrono::seconds(taskTime));
                cout << "[TASK] Task completed.\n";
                };

            pool.addTask(task);

            this_thread::sleep_for(chrono::seconds(3));
        }
        });

    while (true) {
        pool.startAcceptingTasks();
        this_thread::sleep_for(chrono::seconds(30));
        pool.stopAcceptingAndExecute();

        // Execution phase (30 seconds)
        this_thread::sleep_for(chrono::seconds(30));
    }
    pool.stopWorkers();
    for (auto& w : workers) {
        if (w.joinable())
            w.join();
    }
    generator.join();

    return 0;
}
