#pragma once
#include <atomic>
#include <condition_variable>
#include <csv2/notification_queue.hpp>
#include <thread>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>

namespace csv2 {

class task_system {
  const unsigned count_;
  std::vector<std::thread> threads_;
  std::vector<notification_queue> queue_{count_};
  std::atomic<unsigned> index_{0};
  std::atomic_bool no_more_tasks_{false};
  moodycamel::ConcurrentQueue<std::string> rows_;

  friend class reader;

  void run(unsigned i) {
    while (true) {
      std::optional<record_t> op;
      for (unsigned n = 0; n != count_; ++n) {
        if (queue_[(i + n) % count_].try_dequeue(op))
          break;
      }
      if (!op.has_value() && !queue_[i].try_dequeue(op)) {
        if (no_more_tasks_) break; else continue;
      }
      // Enqueue line
      rows_.enqueue(op.value());
    }
  }

public:
  task_system(const unsigned count = std::thread::hardware_concurrency()) 
    : count_(count) {}

  ~task_system() {
    for (auto &thread: threads_)
      thread.join();
  }

  void start() {
    for (unsigned n = 0; n != count_; ++n) {
      threads_.emplace_back([&, n] { run(n); });
    }
  }

  void stop() {
    no_more_tasks_ = true;
  }

  template <typename F> void async_(F &&f) {
    if (count_ == 0) {
      // No worker threads
      // Directly enqueue onto rows_
      rows_.enqueue(std::forward<std::string>(f));
      return;
    } else {
      const auto i = index_++;
      for (unsigned n = 0; n != count_; ++n) {
        if (queue_[(i + n) % count_].enqueue(std::forward<F>(f)))
          return;
      }
      queue_[i % count_].enqueue(std::forward<F>(f));
    }
  }
};

} // namespace iris
