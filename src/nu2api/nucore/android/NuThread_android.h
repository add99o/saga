#pragma once

#include <pthread.h>

#include "nu2api/nucore/common.h"

class NuThreadSemaphore {
  public:
    NuThreadSemaphore(i32 max_signals);
    ~NuThreadSemaphore();

  public:
    void Signal();
    bool TryWait();
    void Wait();

  private:
    pthread_mutex_t mutex;
    pthread_cond_t condition;

    i32 signaled_count;
    i32 max_signals;
};
