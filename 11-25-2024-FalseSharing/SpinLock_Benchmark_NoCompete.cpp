#include <benchmark/benchmark.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_set>
#include "spin_lock.h"

constexpr int kSetSize = 10000;

// struct alignas(64) PaddedCounter {
//   std::atomic<int> value{0};
//   char padding[64 - sizeof(std::atomic<int>)];  // 填充到缓存行大小
// };

struct alignas(64) PaddedCounterLock {
  int value{0};
  char padding[64 - sizeof(std::atomic<int>)];  // 填充到缓存行大小
};

class MyBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State& state) override {
    std::call_once(flag, [this, &state]() {
      for (int i = 0; i < kSetSize; i++) {
        s.insert(i);
      }
      sums_atomic = std::vector<PaddedCounterLock>(state.threads());
      sum_lock = std::vector<PaddedCounterLock>(state.threads());
    });
  }

  const std::unordered_set<int>& GetSet() { return s; }
  std::vector<PaddedCounterLock>& GetSumsAtomic() { return sums_atomic; }
  std::vector<PaddedCounterLock>& GetSumLock() { return sum_lock; }

 private:
  std::unordered_set<int> s;
  std::once_flag flag;
  std::vector<PaddedCounterLock> sums_atomic;
  std::vector<PaddedCounterLock> sum_lock;
};

BENCHMARK_DEFINE_F(MyBenchmark, MultiThreadedWorkAtomic)(benchmark::State& state) {
  SpinLock m;
  for (auto _ : state) {
    int size_sum = kSetSize;
    int size_per_thread = (size_sum + state.threads() - 1) / state.threads();
    int sum = 0;
    int start = state.thread_index() * size_per_thread;
    int end = std::min((state.thread_index() + 1) * size_per_thread, size_sum);
    for (int i = start; i < end; i++) {
      const auto& inst = GetSet();
      if (inst.count(i) > 0) {
        std::lock_guard<SpinLock> lg(m);
        benchmark::DoNotOptimize(GetSumsAtomic()[state.thread_index()].value++);
      }
    }
  }
}

BENCHMARK_DEFINE_F(MyBenchmark, MultiThreadedWorkLock)(benchmark::State& state) {
  std::mutex m;
  for (auto _ : state) {
    int size_sum = kSetSize;
    int size_per_thread = (size_sum + state.threads() - 1) / state.threads();
    int sum = 0;
    int start = state.thread_index() * size_per_thread;
    int end = std::min((state.thread_index() + 1) * size_per_thread, size_sum);
    for (int i = start; i < end; i++) {
      const auto& inst = GetSet();
      if (inst.count(i) > 0) {
        std::lock_guard<std::mutex> lg(m);
        benchmark::DoNotOptimize(GetSumLock()[state.thread_index()].value++);
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
