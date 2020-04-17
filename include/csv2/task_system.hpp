#pragma once
#include <atomic>
#include <condition_variable>
#include <csv2/notification_queue.hpp>
#include <mutex>
#include <thread>
#include <vector>
#include <optional>
#include <iostream>
#include <unordered_map>

namespace csv2 {

using lock_t = std::unique_lock<std::mutex>;

class task_system {
  const unsigned count_;
  std::vector<std::thread> threads_;
  std::vector<notification_queue> queue_{count_};
  std::atomic<unsigned> index_{0};
  std::atomic_bool no_more_tasks_{false};
  std::mutex rows_mutex_;
  std::unordered_map<unsigned, std::string> rows_;

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

      {
        lock_t lock(rows_mutex_);
        rows_.insert(op.value());
      }
    }
  }

public:
  task_system(const unsigned count = std::thread::hardware_concurrency()) 
    : count_(count) {}

  ~task_system() {
    for (auto &thread: threads_)
      thread.join();
    // for (auto& [k,v]: rows_)
    //   std::cout << k << ": " << v;
    // std::cout << rows_.size() << std::endl;
  }

  void start() {
    for (unsigned n = 0; n != count_; ++n) {
      threads_.emplace_back([&, n] { run(n); });
    }
  }

  void stop() {
    no_more_tasks_ = true;
  }

  auto find_row(size_t index) {
    lock_t lock(rows_mutex_);
    return rows_.find(index);
  }

  size_t rows() {
    lock_t lock(rows_mutex_);
    return rows_.size();
  }

  auto rows_end() {
    lock_t lock(rows_mutex_);
    return rows_.end();
  }

  template <typename F> void async_(F &&f) {
    const auto i = index_++;
    for (unsigned n = 0; n != count_; ++n) {
      if (queue_[(i + n) % count_].enqueue(std::forward<F>(f)))
        return;
    }
    queue_[i % count_].enqueue(std::forward<F>(f));
  }
};

} // namespace iris
