#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

struct PaddedSharedData {
    alignas(64) long value1;   // Padding to separate into different cache lines
    alignas(64) long value2;
};

PaddedSharedData padded_shared_data;

void threadFunc1() {
    for (int i = 0; i < 10000000; ++i) {
        ++padded_shared_data.value1;  // Thread 1 updates value1
    }
}

void threadFunc2() {
    for (int i = 0; i < 10000000; ++i) {
        ++padded_shared_data.value2;  // Thread 2 updates value2
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::thread t1(threadFunc1);
    std::thread t2(threadFunc2);

    t1.join();
    t2.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Final value1: " << padded_shared_data.value1 << std::endl;
    std::cout << "Final value2: " << padded_shared_data.value2 << std::endl;
    std::cout << "Elapsed time: " << elapsed.count() << " seconds." << std::endl;

    return 0;
}

