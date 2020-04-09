#ifndef _CONDITION_H_
#define _CONDITION_H_

#include <assert.h>
#include <condition_variable>
#include <mutex>

class CondVar {
  public:
    explicit CondVar(std::mutex &mtx)
        : m_mtx(mtx)
    {
        assert(mtx != nullptr);
    }
    ~CondVar()               = default;
    CondVar(const CondVar &) = delete;
    const CondVar &operator=(const CondVar &) = delete;

    // 等待阻塞
    void Wait()
    {
        std::unique_lock<std::mutex> lock(m_mtx, std::adopt_lock);
        m_cv.wait(lock);
        lock.release();
    }

    // 超时等待
    void WaitSeconds(double seconds)
    {
        const int64_t kNanoSecondsPerSecond = 1000000000;
        int64_t       nanoseconds =
            static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.wait_for(lock, std::chrono::nanoseconds(nanoseconds));
    }

    // 通知
    void Signal() { m_cv.notify_one(); }

    // 通知所有
    void SignalAll() { m_cv.notify_all(); }

  private:
    std::mutex &            m_mtx;
    std::condition_variable m_cv;
};
#endif