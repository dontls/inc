#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace libthread {
enum task_level {
  kLeavel0,
  kLeavel1,
  kLeavel2,
};
class Pool {

public:
  using task = std::function<void()>;
  using task_pair = std::pair<task_level, task>;
  using threads = std::vector<std::thread *>;

  static const int kInitThreadsSize = 3;

  Pool(/* args */) : m_mutex(), isruning_(false) {}

  ~Pool() {
    if (isruning_) {
      Stop();
    }
  }

  void Start() {
    isruning_ = true;
    threads_.reserve(kInitThreadsSize);
    for (int i = 0; i < kInitThreadsSize; i++) {
      threads_.push_back(new std::thread(std::bind(&Pool::threadLoop, this)));
    }
  }
  void Stop() {
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      isruning_ = false;
      cv_.notify_all();
    }

    for (threads::iterator it = threads_.begin(); it != threads_.end(); ++it) {
      (*it)->join();
      delete *it;
    }
    threads_.clear();
  }
  void AddTask(const task &task) {
    std::unique_lock<std::mutex> lock(m_mutex);
    task_pair taskPair(kLeavel2, task);
    tasks_.push(taskPair);
    cv_.notify_one();
  }
  void AddTask(const task_pair &taskPair) {
    std::unique_lock<std::mutex> lock(m_mutex);
    tasks_.push(taskPair);
    cv_.notify_one();
  }

private:
  Pool(const Pool &) = delete; // 禁止复制拷贝
  const Pool &operator=(const Pool &) = delete;

  struct task_priority {
    bool operator()(const Pool::task_pair p1, const Pool::task_pair p2) {
      return p1.first > p2.first;
    }
  };

  using tasks =
      std::priority_queue<task_pair, std::vector<task_pair>, task_priority>;

  void threadLoop() {
    while (isruning_) {
      task t = take();
      if (t) {
        t();
      }
    }
  }
  task take() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (tasks_.empty() && isruning_) {
      cv_.wait(lock);
    }

    task t;
    tasks::size_type size = tasks_.size();
    if (tasks_.empty() && isruning_) {
      t = tasks_.top().second;
      tasks_.pop();
    }
    return t;
  }

private:
  threads threads_;
  tasks tasks_;
  std::mutex m_mutex;
  mutable std::condition_variable cv_;
  bool isruning_;
};
} // namespace libthread
