#ifndef __THREAD_MUTEX_H__
#define __THREAD_MUTEX_H__

#include <mutex>

class CommMutex {
private:
    std::mutex _lock;

public:
    void Lock()
    {
        _lock.lock();
    }

    void UnLock()
    {
        _lock.unlock();
    }
    CommMutex()  = default;
    ~CommMutex() = default;
};

class CommRWLock {
private:
    CommMutex* const _mu;

public:
    explicit CommRWLock(CommMutex* mu) : _mu(mu)
    {
        _mu->Lock();
    }
    ~CommRWLock()
    {
        _mu->UnLock();
    }

    CommRWLock(const CommRWLock&) = delete;
    CommRWLock& operator=(const CommRWLock&) = delete;
};

#endif