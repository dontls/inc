#pragma once

#include <pthread.h>

namespace libcomm {

typedef pthread_mutex_t Mutex;

class Lock {
public:
  explicit Lock(Mutex *lock) : lock_(lock) { pthread_mutex_lock(lock_); }
  ~Lock() { pthread_mutex_unlock(lock_); }

private:
  Mutex *lock_;
};

} // namespace libcomm
