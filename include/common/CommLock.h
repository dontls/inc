#ifndef __COMM_LOCK_H__
#define __COMM_LOCK_H__

#include <pthread.h>

class CommLock {
public:
    void Lock()
    {
        pthread_mutex_lock(&_lock);
    }
    void UnLock()
    {
        pthread_mutex_unlock(&_lock);
    }
    CommLock()
    {
        pthread_mutex_init(&_lock, NULL);
    }
    ~CommLock()
    {
        pthread_mutex_destroy(&_lock);
    }

private:
    pthread_mutex_t _lock;
};
#endif