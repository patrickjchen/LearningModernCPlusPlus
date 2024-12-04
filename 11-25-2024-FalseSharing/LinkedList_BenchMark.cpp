#include <benchmark/benchmark.h>
#include <mutex>
#include <atomic>
#include "ebr.h"
#include "spin_lock.h"

struct Node {
  Node() : lock(), next(nullptr) {}
  int key;
  int value;
  Node *next;
  SpinLock lock;
};

class NodeFree {
 public:
  NodeFree(Node *node) { delete node; }
};

/*
 * 快速测试起见，简单写了个list版本的kv结构，里面只会有3个元素，然后只支持Get和Modify，Modify也必定会命中key。
 * 不是直接把key，value，next设置成atomic变量而是使用SpinLock的原因是模拟复杂情况，真实情况下会存在Add和Remove操作，实现没有如此简单。
 */
class MyList {
 public:
  MyList() {
    Node *pre_node = nullptr;
    auto *&cur_node = root_;
    // 这里虽然插入了10个元素，但后面的实现会假设第一个key 9作为header是绝对不会被修改或者读到的。
    for (int i = 0; i < 10; i++) {
      cur_node = new Node;
      cur_node->key = i;
      cur_node->value = i;
      cur_node->next = pre_node;
      pre_node = cur_node;
    }
  }

  int Get(int key, int *value) {
    root_->lock.lock();
    auto *cur_node = root_->next;
    auto *pre_node = root_;
    while (cur_node != nullptr) {
      cur_node->lock.lock();
      pre_node->lock.unlock();
      if (key == cur_node->key) {
        *value = cur_node->value;
        cur_node->lock.unlock();
        return 0;
      }
      pre_node = cur_node;
      cur_node = cur_node->next;
    }
    pre_node->lock.unlock();
    return 1;
  }
  int Modify(int key, int value) {
    root_->lock.lock();
    auto *cur_node = root_->next;
    auto *pre_node = root_;
    while (cur_node != nullptr) {
      cur_node->lock.lock();
      pre_node->lock.unlock();
      if (key == cur_node->key) {
        cur_node->value = value;
        cur_node->lock.unlock();
        return 0;
      }
      pre_node = cur_node;
      cur_node = cur_node->next;
    }
    pre_node->lock.unlock();
    return 1;
  }

  int GetUseEbr(int key, int *value) {
    ebr_mgr_.StartRead();
    auto *cur_node = root_->next;
    while (cur_node != nullptr) {
      if (key == cur_node->key) {
        *value = cur_node->value;
        ebr_mgr_.EndRead();
        return 0;
      }
      cur_node = cur_node->next;
    }
    ebr_mgr_.EndRead();
    return 1;
  }

  int ModifyUseEbr(int key, int value) {
    root_->lock.lock();
    auto *cur_node = root_->next;
    auto *pre_node = root_;
    while (cur_node != nullptr) {
      cur_node->lock.lock();
      if (key == cur_node->key) {
        auto *new_node = new Node;
        new_node->key = cur_node->key;
        new_node->value = value;
        new_node->next = cur_node->next;
        pre_node->next = new_node;
        cur_node->lock.unlock();
        pre_node->lock.unlock();
        ebr_mgr_.FreeObject(cur_node);
        return 0;
      }
      auto *next_node = cur_node->next;
      pre_node->lock.unlock();
      pre_node = cur_node;
      cur_node = next_node;
    }
    pre_node->lock.unlock();
    return 1;
  }

 private:
  Node *root_;
  EbrManager<Node, NodeFree, 15> ebr_mgr_;
};

class MyBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State &state) override {}

  MyList &GetMyList() { return l; }

 private:
  MyList l;
  std::once_flag flag;
};

constexpr int kKeySize = 10000;

BENCHMARK_DEFINE_F(MyBenchmark, MultiThreadedWorkNoUseEbr)(benchmark::State &state) {
  for (auto _ : state) {
    auto &mylist = GetMyList();
    if (0 == state.thread_index()) {
      // modify
      for (int i = 0; i < kKeySize; i++) {
        mylist.Modify(i % 9, i);
      }
    } else {
      // get
      for (int i = 0; i < kKeySize; i++) {
        int value;
        mylist.Get(i % 9, &value);
      }
    }
  }
}

BENCHMARK_DEFINE_F(MyBenchmark, MultiThreadedWorkUseEbr)(benchmark::State &state) {
  for (auto _ : state) {
    auto &mylist = GetMyList();
    if (0 == state.thread_index()) {
      // modify
      for (int i = 0; i < kKeySize; i++) {
        mylist.ModifyUseEbr(i % 9, i);
      }
    } else {
      // get
      for (int i = 0; i < kKeySize; i++) {
        int value;
        mylist.GetUseEbr(i % 9, &value);
      }
    }
  }
}

// 注册基准测试，并指定线程数
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkNoUseEbr)->Threads(4);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkNoUseEbr)->Threads(8);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkNoUseEbr)->Threads(12);

BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkUseEbr)->Threads(4);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkUseEbr)->Threads(8);
BENCHMARK_REGISTER_F(MyBenchmark, MultiThreadedWorkUseEbr)->Threads(12);

BENCHMARK_MAIN();
