#pragma once

#include <pthread.h>

namespace libthread {

class Lock {
public:
  Lock() {
    pthread_mutex_init(&lock_, NULL);
    pthread_mutex_lock(&lock_);
  }
  ~Lock() {
    pthread_mutex_unlock(&lock_);
    pthread_mutex_destroy(&lock_);
  }

private:
  pthread_mutex_t lock_;
};

} // namespace libthread
