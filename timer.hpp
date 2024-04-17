#pragma once

#include <functional>
#include <thread>

namespace libtime {

class Timer {
private:
  /* data */
  bool m_bClear = false;

public:
  Timer(/* args */) {}
  ~Timer() {}

  // void func(){}
  // Timer timer;
  // timer.SetTimeout(func, 1000);
  // 超时执行
  void SetTimeout(std::function<void()> task, int msDelay) {
    this->m_bClear = false;
    std::thread t([=]() {
      if (this->m_bClear)
        return;
      std::this_thread::sleep_for(std::chrono::microseconds(msDelay));
      if (this->m_bClear)
        return;
      task();
    });
    t.detach();
  }

  // void func(){}
  // Timer timer;
  // timer.SetInterval(func, 1000);
  // 定时执行
  void SetInterval(std::function<void()> task, int msInterval) {
    this->m_bClear = false;
    std::thread t([=]() {
      while (true) {
        if (this->m_bClear)
          return;

        std::this_thread::sleep_for(std::chrono::milliseconds(msInterval));
        if (this->m_bClear)
          return;

        task();
      }
    });
    t.detach();
  }

  // 停止定时
  void Stop() { this->m_bClear = true; }
};
} // namespace libtime
