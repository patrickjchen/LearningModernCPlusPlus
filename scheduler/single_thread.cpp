#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <thread>

struct Task {
    std::chrono::steady_clock::time_point execute_at;
    std::function<void()> func;

    bool operator<(const Task& other) const {
        return execute_at > other.execute_at; // Min-heap based on time
    }
};

class Scheduler {
public:
    void schedule(std::function<void()> func, int delay_seconds) {
        auto execute_time = std::chrono::steady_clock::now() + std::chrono::seconds(delay_seconds);
        tasks_.push({execute_time, func});
    }

    void run() {
        while (!stop_flag) {
            auto now = std::chrono::steady_clock::now();

            // Process tasks that are ready to be executed
            while (!tasks_.empty() && tasks_.top().execute_at <= now) {
                auto task = tasks_.top();
                tasks_.pop();
                task.func(); // Execute the task function
            }

            // Sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void stop() {
        stop_flag = true;
    }

private:
    std::priority_queue<Task> tasks_;
    bool stop_flag = false;
};

int main() {
    Scheduler scheduler;

    // Add tasks to the scheduler
    scheduler.schedule([] { std::cout << "Task 1 executed\n"; }, 2);
    scheduler.schedule([] { std::cout << "Task 2 executed\n"; }, 4);

    // Run the scheduler in the main thread
    std::thread scheduler_thread([&scheduler]() { scheduler.run(); });

    // Dynamically add a new task while the scheduler is running
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    scheduler.schedule([] { std::cout << "Task 3 executed\n"; }, 3);

    std::this_thread::sleep_for(std::chrono::seconds(5)); // Allow time for tasks to complete
    scheduler.stop(); // Stop the scheduler
    scheduler_thread.join();

    return 0;
}

