#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <random>
#include <vector>

using namespace std;

class ThreadPool {
private:
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool accepting = false;
    bool stop = false;
    int workerCount;

public:
    ThreadPool(int count) : workerCount(count) {
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
        cout << "[POOL] Task added to queue. Current queue size: " << tasks.size() << "\n";
    }

    void workerFunction(int id) {
        while (true) {
            function<void()> task;
            {
                unique_lock<mutex> lock(mtx);
                cv.wait(lock, [this] { return stop || !tasks.empty(); });

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
                cout << "[WORKER " << id << "] Executing task...\n";
                task();
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
    }

    void stopWorkers() {
        {
            lock_guard<mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
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
