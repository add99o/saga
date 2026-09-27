#include "nu2api/nuandroid/nuphoneos.h"

#include "nu2api/nucore/common.h"
#include "nu2api/nucore/android/NuThread_android.h"

static PHONEEVENTCALLBACK *s_phoneOSEventCallbacks[7];

i32 g_systemPauseReceived;
i32 g_systemResumeReceived;
i32 g_systemDidBecomeActiveReceived;

struct PhoneOSMessageQueue {
    NuThreadSemaphore free_slots;
    NuThreadSemaphore pending;
    NuThreadSemaphore processed;
    NuThreadSemaphore empty;
    NuThreadSemaphore occupied;
    u32 head;
    u32 tail;
    struct Entry {
        NuPhoneOSMessage message;
        u32 completion_id;
    } entries[128];
    u32 completion_id;

    PhoneOSMessageQueue()
        : free_slots(128), pending(128), processed(1), empty(1), occupied(1), head(0), tail(0),
          completion_id(0x0fffffff) {
        for (i32 index = 0; index < 128; ++index)
            free_slots.Signal();
    }
};
DECOMP_ASSERT(sizeof(PhoneOSMessageQueue::Entry) == 0x1c, "PhoneOS queue entry ABI");
DECOMP_ASSERT(sizeof(PhoneOSMessageQueue) == 0xe5c, "PhoneOS queue ABI");
static PhoneOSMessageQueue s_phoneOSMessageQueue;

void NuPhoneOSRegisterEventCallback(i32 type, PHONEEVENTCALLBACK *callback_fn) {
    s_phoneOSEventCallbacks[type] = callback_fn;
}

extern "C" void NuPhoneOSMessagePost(const NuPhoneOSMessage *message, i32 nonblocking, i32 wait_until_processed) {
    if (nonblocking != 0) {
        if (!s_phoneOSMessageQueue.free_slots.TryWait())
            return;
    } else {
        s_phoneOSMessageQueue.free_slots.Wait();
    }

    PhoneOSMessageQueue::Entry &entry = s_phoneOSMessageQueue.entries[s_phoneOSMessageQueue.head & 127];
    entry.message = *message;
    entry.completion_id = 0x0fffffff;
    if (s_phoneOSMessageQueue.head == s_phoneOSMessageQueue.tail) {
        s_phoneOSMessageQueue.empty.TryWait();
        s_phoneOSMessageQueue.occupied.TryWait();
        s_phoneOSMessageQueue.occupied.Signal();
    }
    ++s_phoneOSMessageQueue.head;
    s_phoneOSMessageQueue.pending.Signal();
    if (wait_until_processed != 0 && s_phoneOSMessageQueue.tail != s_phoneOSMessageQueue.head)
        s_phoneOSMessageQueue.processed.Wait();
}

extern "C" void NuPhoneOSMessagePump(void) {
    if (g_systemPauseReceived != 0) {
        if (s_phoneOSEventCallbacks[PHONE_EVENT_PAUSE] != NULL)
            s_phoneOSEventCallbacks[PHONE_EVENT_PAUSE](NULL);
        g_systemPauseReceived = 0;
    }
    if (g_systemResumeReceived != 0) {
        if (s_phoneOSEventCallbacks[PHONE_EVENT_RESUME] != NULL)
            s_phoneOSEventCallbacks[PHONE_EVENT_RESUME](NULL);
        g_systemResumeReceived = 0;
    }
    if (g_systemDidBecomeActiveReceived != 0) {
        if (s_phoneOSEventCallbacks[PHONE_EVENT_BECOME_ACTIVE] != NULL)
            s_phoneOSEventCallbacks[PHONE_EVENT_BECOME_ACTIVE](NULL);
        g_systemDidBecomeActiveReceived = 0;
    }

    while (s_phoneOSMessageQueue.pending.TryWait()) {
        const PhoneOSMessageQueue::Entry entry = s_phoneOSMessageQueue.entries[s_phoneOSMessageQueue.tail & 127];
        if (entry.completion_id == s_phoneOSMessageQueue.completion_id)
            s_phoneOSMessageQueue.processed.Signal();
        ++s_phoneOSMessageQueue.tail;
        if (s_phoneOSMessageQueue.tail == s_phoneOSMessageQueue.head) {
            s_phoneOSMessageQueue.occupied.TryWait();
            s_phoneOSMessageQueue.empty.TryWait();
            s_phoneOSMessageQueue.empty.Signal();
        }
        s_phoneOSMessageQueue.free_slots.Signal();
        if (s_phoneOSEventCallbacks[entry.message.type] != NULL)
            s_phoneOSEventCallbacks[entry.message.type](&entry.message.data);
    }
}
