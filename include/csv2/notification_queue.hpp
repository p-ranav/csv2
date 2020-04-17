#pragma once
#include <condition_variable>
#include <deque>
#include <functional>
#include <utility>
#include <string>
#include <optional>
#include <csv2/external/concurrentqueue/concurrentqueue.h>

namespace csv2 {

using record_t = std::pair<unsigned, std::string>;

class notification_queue {
  moodycamel::ConcurrentQueue<record_t> queue_;
  moodycamel::ProducerToken ptok{queue_};
  moodycamel::ConsumerToken ctok{queue_};

public:
  bool try_dequeue(std::optional<record_t> &op) {
    return queue_.try_dequeue(ctok, op);
  }

  template <typename Record> 
  bool enqueue(Record &&record) {
    return queue_.enqueue(ptok, record);
  }
};

} // namespace csv2
