#include "nu2api/nuandroid/nuphoneos.h"

#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuthreadqueue.h"

static PHONEEVENTCALLBACK *s_phoneOSEventCallbacks[7];
typedef NuThreadQueue<NuPhoneOSMessage, 128> NuPhoneOSQueue;
static NuPhoneOSQueue s_phoneOSMessageQueue;

DECOMP_ASSERT(sizeof(NuPhoneOSQueue) == 0xe5c, "PhoneOS message queue target layout");
DECOMP_ASSERT(offsetof(NuPhoneOSQueue, write_count) == 0x50, "PhoneOS queue write counter");
DECOMP_ASSERT(offsetof(NuPhoneOSQueue, read_count) == 0x54, "PhoneOS queue read counter");
DECOMP_ASSERT(offsetof(NuPhoneOSQueue, records) == 0x58, "PhoneOS queue records");
DECOMP_ASSERT(offsetof(NuPhoneOSQueue, waiting_token) == 0xe58, "PhoneOS queue token");

i32 g_systemPauseReceived;
i32 g_systemResumeReceived;
i32 g_systemDidBecomeActiveReceived;

void NuPhoneOSRegisterEventCallback(i32 type, PHONEEVENTCALLBACK *callback_fn) {
    s_phoneOSEventCallbacks[type] = callback_fn;
}

extern "C" void NuPhoneOSMessagePost(const NuPhoneOSMessage *message, i32 nonblocking, i32 wait_until_processed) {
    if (nonblocking != 0) {
        if (!s_phoneOSMessageQueue.TryPost(*message))
            return;
    } else {
        s_phoneOSMessageQueue.Post(*message);
    }
    if (wait_until_processed != 0)
        s_phoneOSMessageQueue.WaitUntilEmpty();
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
    NuPhoneOSMessage message;
    while (s_phoneOSMessageQueue.TryPop(message)) {
        if (s_phoneOSEventCallbacks[message.type] != NULL)
            s_phoneOSEventCallbacks[message.type](&message.data);
    }
}
