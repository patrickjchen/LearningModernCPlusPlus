#include <coroutine>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <atomic>
#include <memory>

struct Task {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type {
        Task get_return_object() { 
            return Task{handle_type::from_promise(*this), std::chrono::steady_clock::now()};
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { 
            return {}; 
        }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    handle_type handle;
    std::chrono::steady_clock::time_point start_time;
    bool completed = false;

    Task(handle_type h, std::chrono::steady_clock::time_point t) : handle(h), start_time(t) {}

    Task(Task&& other) noexcept : handle(other.handle), start_time(other.start_time), completed(other.completed) {
        other.handle = nullptr;
    }

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            start_time = other.start_time;
            completed = other.completed;
            other.handle = nullptr;
        }
        return *this;
    }

    ~Task() { if (handle) handle.destroy(); }

    bool resume() { 
        if (handle) {
            handle.resume();
            return true;
        }
        return false;
    }

    bool is_completed() const { return completed; }
};

// Comparator for priority queue based on start time, using unique_ptr
struct TaskComparator {
    bool operator()(const std::unique_ptr<Task>& lhs, const std::unique_ptr<Task>& rhs) const {
        return lhs->start_time > rhs->start_time; // Earlier tasks have higher priority
    }
};

// Scheduler class with priority queue
class Scheduler {
public:
    Scheduler(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this]() { worker_thread(); });
        }
    }

    ~Scheduler() {
        stop();
    }

    void schedule(Task&& task, int delay_ms) {
        std::unique_lock lock(mutex_);
        task.start_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
        tasks_.emplace(std::make_unique<Task>(std::move(task)));
        cv_.notify_one();
    }

    void stop() {
        {
            std::unique_lock lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    void worker_thread() {
        while (true) {
            std::unique_ptr<Task> task;

            {
                std::unique_lock lock(mutex_);
                cv_.wait(lock, [this]() {
                    return stop_ || (!tasks_.empty() && tasks_.top()->start_time <= std::chrono::steady_clock::now());
                });

                if (stop_ && tasks_.empty()) {
                    break;
                }

                task = std::move(const_cast<std::unique_ptr<Task>&>(tasks_.top())); // Move task out safely
                tasks_.pop();
            }

            if (task && task->resume()) {
                if (task->handle.done()) {
                    task->completed = true; // Mark as completed once coroutine is done
                } else {
                    std::unique_lock relock(mutex_);
                    task->start_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
                    tasks_.emplace(std::move(task)); // Re-add to the queue if needed
                }
            }
        }
    }

    std::priority_queue<std::unique_ptr<Task>, std::vector<std::unique_ptr<Task>>, TaskComparator> tasks_;
    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
};

unsigned thread_id() {
  auto id = std::this_thread::get_id();
  return *(unsigned *)&id % 100;
}

// Coroutine function for tasks
Task myCoroutine(int id, int duration_ms) {
    int elapsed = 0;
    const int interval = 100; // Interval for printing messages

    while (elapsed < duration_ms) {
        std::cout << "Task " << id << " running in thread " << thread_id() <<" running at " << elapsed << " ms\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        elapsed += interval;
        co_await std::suspend_always{};
    }

    std::cout << "Task " << id << " completed.\n";
}

int main() {
    constexpr size_t num_threads = 4;
    Scheduler scheduler(num_threads);

    // Schedule tasks with different delays and durations
    scheduler.schedule(myCoroutine(1, 500), 200); // Task 1: 500 ms, start after 200 ms
    scheduler.schedule(myCoroutine(2, 700), 500); // Task 2: 700 ms, start after 500 ms
    scheduler.schedule(myCoroutine(3, 900), 1000); // Task 3: 900 ms, start after 1000 ms
    scheduler.schedule(myCoroutine(4, 300), 300); // Task 4: 300 ms, start after 300 ms

    scheduler.schedule(myCoroutine(5, 500), 200); // Task 5: 500 ms, start after 200 ms
    scheduler.schedule(myCoroutine(6, 700), 500); // Task 6: 700 ms, start after 500 ms
    scheduler.schedule(myCoroutine(7, 900), 1000); // Task 7: 900 ms, start after 1000 ms
    scheduler.schedule(myCoroutine(8, 300), 300); // Task 8: 300 ms, start after 300 ms

    // Allow some time for tasks to complete before stopping the scheduler
    std::this_thread::sleep_for(std::chrono::seconds(3));
    scheduler.stop();

    return 0;
}

