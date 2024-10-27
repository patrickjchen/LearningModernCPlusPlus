#include <coroutine>
#include <iostream>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

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

// Scheduler class
class Scheduler {
public:
    void schedule(Task&& task) {
        std::unique_lock lock(mutex_);
        tasks_.push_back(std::move(task));
        cv_.notify_one();
    }

    void run() {
        while (true) {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this]() { return !tasks_.empty() || stop_; });

            if (tasks_.empty() && stop_) {
                break;
            }

            // Move the task out of the queue
            auto task = std::move(tasks_.front());
            tasks_.pop_front();

            // Unlock the mutex while resuming the task
            lock.unlock();

            if (task.resume()) {
                std::unique_lock relock(mutex_);
                tasks_.push_back(std::move(task));
            }
        }
    }

    void stop() {
        std::unique_lock lock(mutex_);
        stop_ = true;
        cv_.notify_all();
    }

private:
    std::deque<Task> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

// Coroutine function for tasks
Task task(Scheduler& scheduler, int id) {
    for (int i = 0; i < 3; ++i) {
        std::cout << "Task " << id << " iteration " << i << std::endl;
        co_await std::suspend_always{};
    }
}

int main() {
    Scheduler scheduler;

    // Schedule initial tasks
    scheduler.schedule(task(scheduler, 1));
    scheduler.schedule(task(scheduler, 2));

    // Run scheduler in a separate thread
    std::thread scheduler_thread([&scheduler]() { scheduler.run(); });

    // Simulate adding more tasks dynamically
    std::this_thread::sleep_for(std::chrono::seconds(1));
    scheduler.schedule(task(scheduler, 3));
    scheduler.schedule(task(scheduler, 4));

    // Allow time for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Stop the scheduler and join the thread
    scheduler.stop();
    scheduler_thread.join();

    return 0;
}

