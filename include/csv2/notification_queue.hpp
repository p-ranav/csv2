#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <utility>
#include <string>
#include <optional>

namespace csv2 {

using lock_t = std::unique_lock<std::mutex>;
using record_t = std::pair<unsigned, std::string>;

class notification_queue {
  std::deque<record_t> queue_;
  bool done_{false};
  std::mutex mutex_;
  std::condition_variable ready_;

public:
  bool try_pop(std::optional<record_t> &op) {
    lock_t lock{mutex_, std::try_to_lock};
    if (!lock || queue_.empty())
      return false;
    op = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  template <typename Function> bool try_push(Function &&fn) {
    {
      lock_t lock{mutex_, std::try_to_lock};
      if (!lock)
        return false;
      queue_.emplace_back(std::forward<Function>(fn));
    }
    ready_.notify_one();
    return true;
  }

  void done() {
    {
      lock_t lock{mutex_};
      done_ = true;
    }
    ready_.notify_all();
  }

  bool pop(record_t &op) {
    lock_t lock{mutex_};
    while (queue_.empty())
      ready_.wait(lock);
    if (queue_.empty())
      return false;
    op = std::move(queue_.front());
    queue_.pop_front();
    return true;
  }

  template <typename Function> void push(Function &&fn) {
    {
      lock_t lock{mutex_};
      queue_.emplace_back(std::function<Function>(fn));
    }
    ready_.notify_one();
  }
};

} // namespace csv2
