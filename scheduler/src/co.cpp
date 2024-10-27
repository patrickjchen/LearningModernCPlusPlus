#include <iostream>
#include <coroutine>

struct SimpleTask {
    struct promise_type {
        SimpleTask get_return_object() {
            return SimpleTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() { return {}; } // Start without initial suspension
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}

        promise_type() = default;
    };

    std::coroutine_handle<promise_type> handle;

    SimpleTask(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~SimpleTask() {
        if (handle) handle.destroy();
    }
    void resume() { handle.resume(); }
};

SimpleTask myCoroutine() {
    std::cout << "Hello, ";
    co_await std::suspend_always{}; // Suspend after printing "Hello, "
    std::cout << "World!\n";
}

int main() {
    auto coroutine = myCoroutine();
    coroutine.resume(); // Only one resume needed now to print "World!"
    return 0;
}

