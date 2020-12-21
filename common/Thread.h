
#ifndef __COMMTHREAD_H__
#define __COMMTHREAD_H__

#include <pthread.h>

class CommThread {
public:
    CommThread() {}
    ~CommThread() {}
    void start()
    {
        pthread_create(&_thread, NULL, CommThread::privStart, this);
    }

    void quit()
    {
        pthread_cancel(_thread);
    }

    void stop()
    {
        pthread_join(_thread, NULL);
    }

    void* currentThreadId()
    {
        return ( void* )pthread_self();
    }

protected:
    virtual void run() = 0;

private:
    static void* privStart(void* arg)
    {
        CommThread* pthis = ( CommThread* )arg;
        pthis->run();
        pthread_exit(NULL);
    }
    pthread_t _thread;
};
#endif