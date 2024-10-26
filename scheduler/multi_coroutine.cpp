#include <coroutine>
#include <iostream>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <atomic>

// Coroutine task class
class Task {
public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    Task(handle_type h) : handle(h) {}
    Task(Task&& t) noexcept : handle(t.handle) { t.handle = nullptr; }
    Task& operator=(Task&& t) noexcept {
        if (this != &t) {
            if (handle) handle.destroy();
            handle = t.handle;
            t.handle = nullptr;
        }
        return *this;
    }
    ~Task() { if (handle) handle.destroy(); }

    bool resume() {
        if (!handle.done()) {
            handle.resume();
            return true;
        }
        return false;
    }

    struct promise_type {
        auto get_return_object() { return Task{handle_type::from_promise(*this)}; }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

private:
    handle_type handle;
};

// Scheduler class with thread pool
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

    void schedule(Task&& task, int start_delay_ms) {
        std::unique_lock lock(mutex_);
        auto scheduled_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(start_delay_ms);
        tasks_.emplace_back(std::move(task), scheduled_time);
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
    using ScheduledTask = std::pair<Task, std::chrono::steady_clock::time_point>;

    void worker_thread() {
        while (true) {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this]() { return !tasks_.empty() || stop_; });

            if (tasks_.empty() && stop_) {
                break;
            }

            // Check if the next task is ready to run
            auto now = std::chrono::steady_clock::now();
            if (tasks_.front().second > now) {
                cv_.wait_until(lock, tasks_.front().second);
            }

            // Move the task directly from the deque to avoid uninitialized variable
            auto scheduled_task = std::move(tasks_.front());
            tasks_.pop_front();

            // Unlock the mutex while resuming the task
            lock.unlock();

            // Delay before the task starts running
            if (scheduled_task.first.resume()) {
                std::unique_lock relock(mutex_);
                tasks_.emplace_back(std::move(scheduled_task));
            }
        }
    }

    std::deque<ScheduledTask> tasks_;
    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
};

unsigned thread_id() {
  auto id = std::this_thread::get_id();
  return *(unsigned *)&id;
}

// Coroutine function for tasks
Task task(Scheduler& scheduler, int id, int duration_ms) {
    int elapsed = 0;
    const int interval = 100;  // Interval for printing messages in milliseconds

    while (elapsed < duration_ms) {
        std::cout << "Task " << id << " Thread "<<thread_id()%10<<" running at " << elapsed << " ms\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        elapsed += interval;
        co_await std::suspend_always{};
    }

    std::cout << "Task " << id << " Thread "<< thread_id() % 10 << " completed.\n";
}

int main() {
    constexpr size_t num_threads = 4;
    Scheduler scheduler(num_threads);

    // Schedule tasks with varying start delays and durations
    scheduler.schedule(task(scheduler, 1, 500), 200);  // Task 1 runs for 500ms, starts after 200ms
    scheduler.schedule(task(scheduler, 2, 700), 500);  // Task 2 runs for 700ms, starts after 500ms
    scheduler.schedule(task(scheduler, 3, 900), 1000); // Task 3 runs for 900ms, starts after 1000ms
    scheduler.schedule(task(scheduler, 4, 300), 300);  // Task 4 runs for 300ms, starts after 300ms

    scheduler.schedule(task(scheduler, 5, 500), 200);  // Task 5 runs for 500ms, starts after 200ms
    scheduler.schedule(task(scheduler, 6, 700), 500);  // Task 6 runs for 700ms, starts after 500ms
    scheduler.schedule(task(scheduler, 7, 900), 1000); // Task 7 runs for 900ms, starts after 1000ms
    scheduler.schedule(task(scheduler, 8, 300), 300);  // Task 8 runs for 300ms, starts after 300ms

    // Allow some time for tasks to complete before stopping the scheduler
    std::this_thread::sleep_for(std::chrono::seconds(3));
    scheduler.stop();

    return 0;
}

