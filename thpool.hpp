#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
public:
  static const int kInitThreadsSize = 3;
  enum taskPriorityE {
    kLeavel0,
    kLeavel1,
    kLeavel2,
  };
  typedef std::function<void()> Task;
  typedef std::pair<taskPriorityE, Task> TaskPair;
  ThreadPool(/* args */) : m_mutex(), m_isStarted(false) {}

  ~ThreadPool() {
    if (m_isStarted) {
      Stop();
    }
  }

  void Start() {
    m_isStarted = true;
    m_threads.reserve(kInitThreadsSize);
    for (int i = 0; i < kInitThreadsSize; i++) {
      m_threads.push_back(
          new std::thread(std::bind(&ThreadPool::threadLoop, this)));
    }
  }
  void Stop() {
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_isStarted = false;
      cv.notify_all();
    }

    for (Threads::iterator it = m_threads.begin(); it != m_threads.end();
         ++it) {
      (*it)->join();
      delete *it;
    }
    m_threads.clear();
  }
  void AddTask(const Task &task) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TaskPair taskPair(kLeavel2, task);
    m_tasks.push(taskPair);
    cv.notify_one();
  }
  void AddTask(const TaskPair &taskPair) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasks.push(taskPair);
    cv.notify_one();
  }

private:
  ThreadPool(const ThreadPool &) = delete; // 禁止复制拷贝
  const ThreadPool &operator=(const ThreadPool &) = delete;

  struct TaskPriorityCmp {
    bool operator()(const ThreadPool::TaskPair p1,
                    const ThreadPool::TaskPair p2) {
      return p1.first > p2.first;
    }
  };

  void threadLoop() {
    while (m_isStarted) {
      Task task = take();
      if (task) {
        task();
      }
    }
  }
  Task take() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_tasks.empty() && m_isStarted) {
      cv.wait(lock);
    }

    Task task;
    Tasks::size_type size = m_tasks.size();
    if (m_tasks.empty() && m_isStarted) {
      task = m_tasks.top().second;
      m_tasks.pop();
    }
    return task;
  }
  typedef std::vector<std::thread *> Threads;
  typedef std::priority_queue<TaskPair, std::vector<TaskPair>, TaskPriorityCmp>
      Tasks;

  Threads m_threads;
  Tasks m_tasks;
  std::mutex m_mutex;
  mutable std::condition_variable cv;
  bool m_isStarted;
};
#endif