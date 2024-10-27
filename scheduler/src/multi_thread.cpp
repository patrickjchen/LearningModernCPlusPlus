#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct Task {
    std::chrono::steady_clock::time_point execute_at;
    int duration_ms;
    std::function<void()> func;

    bool operator<(const Task& other) const {
        return execute_at > other.execute_at; // Min-heap based on time
    }
};

class MultiThreadScheduler {
public:
    MultiThreadScheduler(int thread_count) : stop_flag(false) {
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([this]() { this->worker_thread(); });
        }
    }

    ~MultiThreadScheduler() {
        stop();
    }

    void schedule(std::function<void()> func, int delay_ms, int duration_ms) {
        auto execute_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push({execute_time, duration_ms, func});
        }
        cv_.notify_one(); // Notify a thread to check for new tasks
    }

    void stop() {
        stop_flag = true;
        cv_.notify_all(); // Wake up all threads to exit
        for (std::thread &thread : threads) {
            if (thread.joinable()) thread.join();
        }
    }

private:
    std::priority_queue<Task> tasks_;
    std::vector<std::thread> threads;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_flag;

    void worker_thread() {
        while (!stop_flag) {
            Task current_task;

            {
                std::unique_lock<std::mutex> lock(mutex_);

                // Wait until the next task's execution time or a new task is added
                if (tasks_.empty() || !cv_.wait_until(lock, tasks_.top().execute_at, [this] {
                        return stop_flag || (!tasks_.empty() && tasks_.top().execute_at <= std::chrono::steady_clock::now());
                    })) {
                    continue;
                }

                // Check stop condition after waking up
                if (stop_flag) return;

                // Pop the next task to execute
                current_task = tasks_.top();
                tasks_.pop();
            }

            // Task execution with time slicing
            auto start_time = std::chrono::steady_clock::now();
            int elapsed_ms = 0;
            while (elapsed_ms < current_task.duration_ms) {
                current_task.func(); // Execute the task action

                // Yield control every 100 milliseconds
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::this_thread::yield(); // Yield to allow interleaving with other tasks

                // Calculate elapsed time for this task
                elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
            }
        }
    }
};

int main() {
    MultiThreadScheduler scheduler(4); // Create a scheduler with 4 threads

    // Schedule tasks with delays and durations
    scheduler.schedule([] { std::cout << "Task 1 running\n"; }, 0, 500); // Runs for 500 ms
    scheduler.schedule([] { std::cout << "Task 3 running\n"; }, 300, 900); // Runs for 900 ms
    scheduler.schedule([] { std::cout << "Task 2 running\n"; }, 100, 700); // Runs for 700 ms
    scheduler.schedule([] { std::cout << "Task 4 running\n"; }, 200, 500); // Runs for 700 ms

    // Allow time for tasks to complete and observe interleaving
    std::this_thread::sleep_for(std::chrono::seconds(3));
    scheduler.stop(); // Stop the scheduler

    return 0;
}

