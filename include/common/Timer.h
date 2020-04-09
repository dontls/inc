#ifndef __TIMER_H__
#define __TIMER_H__

#include <functional>
#include <iostream>
#include <thread>

class CommTimer {
  private:
    /* data */
    bool m_bClear = false;

  public:
    CommTimer(/* args */) {}
    ~CommTimer() {}

    // void func(){}
    // Timer timer;
    // timer.SetTimeout(func, 1000);
    // 超时执行
    void SetTimeout(std::function<void()> task, int msDelay)
    {
        this->m_bClear = false;
        std::thread t([=]() {
            if (this->m_bClear) return;
            std::this_thread::sleep_for(std::chrono::microseconds(msDelay));
            if (this->m_bClear) return;
            task();
        });
        t.detach();
    }

    // void func(){}
    // Timer timer;
    // timer.SetInterval(func, 1000);
    // 定时执行
    void SetInterval(std::function<void()> task, int msInterval)
    {
        this->m_bClear = false;
        std::thread t([=]() {
            while (true) {
                if (this->m_bClear) return;

                std::this_thread::sleep_for(
                    std::chrono::milliseconds(msInterval));
                if (this->m_bClear) return;

                task();
            }
        });
        t.detach();
    }

    // 停止定时
    void Stop() { this->m_bClear = true; }
};
#endif