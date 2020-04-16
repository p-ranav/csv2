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

class task_system {
  const unsigned count_{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads_;
  std::vector<notification_queue> queue_{count_};
  std::atomic<unsigned> index_{0};
  std::atomic_bool done_{0};
  std::mutex queue_mutex_;
  std::condition_variable ready_;

  std::mutex rows_mutex_;
  std::unordered_map<unsigned, std::string> rows_;

  void run(unsigned i) {
    while (true) {
      std::optional<record_t> op;
      for (unsigned n = 0; n != count_; ++n) {
        if (queue_[(i + n) % count_].try_pop(op))
          break;
      }
      if (!op.has_value() && !queue_[i].try_pop(op)) {
        if (done_) break; else continue;
      }
      // Use record_t op
      {
        lock_t lock{rows_mutex_};
        rows_.insert(op.value());
      }
      // rows_.insert(op.value());
      // std::cout << "Received row: " << op.value().first << " " << op.value().second << "\n";
    }
  }

public:
  task_system(const unsigned count = std::thread::hardware_concurrency()) : count_(count) {}

  ~task_system() {
    for (auto &queue : queue_)
      queue.done();
    for (auto &thread: threads_)
      thread.join();
    std::cout << rows_.size() << std::endl;
  }

  void start() {
    for (unsigned n = 0; n != count_; ++n) {
      threads_.emplace_back([&, n] { run(n); });
    }
  }

  void stop() {
    std::cout << "Task system stopped\n";
    done_ = true;
  }

  template <typename F> void async_(F &&f) {
    while (!done_) {
      auto i = index_++;
      for (unsigned n = 0; n != count_; ++n) {
        if (queue_[(i + n) % count_].try_push(std::forward<F>(f))) {
          // ready_.notify_one();
          return;
        }
      }
      if (queue_[i % count_].try_push(std::forward<F>(f))) {
        // ready_.notify_one();
        return;
      }
      index_ = 0;
    }
  }
};

} // namespace iris
