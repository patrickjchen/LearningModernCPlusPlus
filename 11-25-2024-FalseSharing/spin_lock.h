#pragma once

#include <atomic>
#include <cstdint>

class Sleeper {
  static const uint32_t kMaxActiveSpin = 4000;
  uint32_t spin_count_;

 public:
  constexpr Sleeper() noexcept : spin_count_(0) {}

  inline __attribute__((always_inline)) static void sleep() noexcept {
    struct timespec ts = {0, 500000};
    nanosleep(&ts, nullptr);
  }

  inline __attribute__((always_inline)) void wait() noexcept {
    if (spin_count_ < kMaxActiveSpin) {
      ++spin_count_;
#ifdef __x86_64__
      asm volatile("pause" ::: "memory");
#elif defined(__aarch64__)
      asm volatile("yield" ::: "memory");
#else
      // Fallback for other architectures
#endif

    } else {
      sleep();
    }
  }
};

class SpinLock {
  enum { FREE = 0, LOCKED = 1 };

 public:
  constexpr SpinLock() : lock_(FREE) {}

  inline __attribute__((always_inline)) bool try_lock() noexcept { return cas(FREE, LOCKED); }

  inline __attribute__((always_inline)) void lock() noexcept {
    Sleeper sleeper;
    while (!try_lock()) {
      do {
        sleeper.wait();
      } while (AtomicCast(&lock_)->load(std::memory_order_relaxed) == LOCKED);
    }
  }

  inline __attribute__((always_inline)) void unlock() noexcept {
    AtomicCast(&lock_)->store(FREE, std::memory_order_release);
  }

 private:
  inline __attribute__((always_inline)) bool cas(uint8_t compare, uint8_t new_val) noexcept {
    return AtomicCast(&lock_)->compare_exchange_strong(compare, new_val, std::memory_order_acquire,
                                                       std::memory_order_relaxed);
  }

  inline __attribute__((always_inline)) static std::atomic<uint8_t>* AtomicCast(uint8_t* value) {
    return reinterpret_cast<std::atomic<uint8_t>*>(value);
  }

 private:
  uint8_t lock_;
};
