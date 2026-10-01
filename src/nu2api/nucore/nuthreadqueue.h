#pragma once

#include "nu2api/nucore/android/NuThread_android.h"

// Fixed-capacity single-producer/single-consumer queue. The semaphores
// publish records and release slots; the counters are also inspected by the
// opposite thread when updating the empty/nonempty notifications.
template <class T, u32 Capacity> class NuThreadQueue {
  public:
    enum { NO_TOKEN = 0x0fffffff };
    struct Record {
        T message;
        u32 token;
    };

    NuThreadSemaphore free_slots;
    NuThreadSemaphore queued_items;
    NuThreadSemaphore token_received;
    NuThreadSemaphore became_empty;
    NuThreadSemaphore became_nonempty;
    u32 write_count;
    u32 read_count;
    Record records[Capacity];
    u32 waiting_token;

    NuThreadQueue()
        : free_slots(Capacity), queued_items(Capacity), token_received(1), became_empty(1), became_nonempty(1),
          write_count(0), read_count(0), waiting_token(NO_TOKEN) {
        static_assert(Capacity != 0 && Capacity <= 0x7fffffffU && (Capacity & (Capacity - 1)) == 0,
                      "queue capacity must be a power of two representable by a semaphore");
        for (u32 i = 0; i < Capacity; ++i)
            free_slots.Signal();
    }

    NuThreadQueue(const NuThreadQueue &) = delete;
    NuThreadQueue &operator=(const NuThreadQueue &) = delete;

    bool TryPost(T message) {
        if (!free_slots.TryWait())
            return false;
        Store(message);
        return true;
    }

    void Post(T message) {
        free_slots.Wait();
        Store(message);
    }

    bool TryPop(T &message) {
        if (!queued_items.TryWait())
            return false;
        message = records[Load(read_count) & (Capacity - 1)].message;
        const u32 token = records[Load(read_count) & (Capacity - 1)].token;
        if (token != NO_TOKEN && token == Load(waiting_token))
            token_received.Signal();
        Save(read_count, Load(read_count) + 1U);
        if (Load(read_count) == Load(write_count)) {
            became_nonempty.TryWait();
            became_empty.TryWait();
            became_empty.Signal();
        }
        free_slots.Signal();
        return true;
    }

    // Retail acknowledges removal from the queue, before its consumer callback.
    void WaitUntilEmpty() {
        if (Load(read_count) != Load(write_count))
            became_empty.Wait();
    }

  private:
    // Relaxed atomic accesses preserve the original x86 loads/stores while
    // avoiding C++ data races on counters shared by the two threads. Record
    // visibility is synchronized by the existing semaphore operations.
    static u32 Load(const u32 &value) {
        return __atomic_load_n(&value, __ATOMIC_RELAXED);
    }
    static void Save(u32 &value, u32 next) {
        __atomic_store_n(&value, next, __ATOMIC_RELAXED);
    }
    void Store(T message) {
        Record record = {message, NO_TOKEN};
        records[Load(write_count) & (Capacity - 1)] = record;
        if (Load(read_count) == Load(write_count)) {
            became_empty.TryWait();
            became_nonempty.TryWait();
            became_nonempty.Signal();
        }
        Save(write_count, Load(write_count) + 1U);
        queued_items.Signal();
    }
};
