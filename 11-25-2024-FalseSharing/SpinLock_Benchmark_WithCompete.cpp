#include <benchmark/benchmark.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_set>
#include "spin_lock.h"

constexpr int kSetSize = 10000;

class MyBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State& state) override {
    std::call_once(flag, [this, &state]() {
      for (int i = 0; i < kSetSize; i++) {
        s.insert(i);
      }
      count_ = 0;
    });
  }

  const std::unordered_set<int>& GetSet() { return s; }
  void SpinLockAndAdd() {
    std::lock_guard<SpinLock> lg(m1_);
    count_++;
  }

  void MutexLockAndAdd() {
    std::lock_guard<std::mutex> lg(m2_);
    count_++;
  }

 private:
  std::unordered_set<int> s;
  std::once_flag flag;
  uint32_t count_;
  SpinLock m1_;
  std::mutex m2_;
};

BENCHMARK_DEFINE_F(MyBenchmark, MultiThreadedWorkAtomic)(benchmark::State& state) {
  for (auto _ : state) {
    int size_sum = kSetSize;
    int size_per_thread = (size_sum + state.threads() - 1) / state.threads();
    int sum = 0;
    int start = state.thread_index() * size_per_thread;
    int end = std::min((state.thread_index() + 1) * size_per_thread, size_sum);
    for (int i = start; i < end; i++) {
      const auto& inst = GetSet();
      if (inst.count(i) > 0) {
        SpinLockAndAdd();
      }
    }
  }
}

BENCHMARK_DEFINE_F(MyBenchmark, MultiThreadedWorkLock)(benchmark::State& state) {
  for (auto _ : state) {
    int size_sum = kSetSize;
    int size_per_thread = (size_sum + state.threads() - 1) / state.threads();
    int sum = 0;
    int start = state.thread_index() * size_per_thread;
    int end = std::min((state.thread_index() + 1) * size_per_thread, size_sum);
    for (int i = start; i < end; i++) {
      const auto& inst = GetSet();
      if (inst.count(i) > 0) {
        MutexLockAndAdd();
      }
    }
  }
}

// 注册基准测试，并指定线程数
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkAtomic)->Threads(1);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkAtomic)->Threads(2);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkAtomic)->Threads(4);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkAtomic)->Threads(8);

BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkLock)->Threads(1);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkLock)->Threads(2);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkLock)->Threads(4);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkLock)->Threads(8);

BENCHMARK_MAIN();
