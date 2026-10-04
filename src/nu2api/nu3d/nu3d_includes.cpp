#include <new>
#include <string.h>

#include "nu2api/nucore/NuDynamicLight.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nucore/nuvuvec.hpp"
#include "nu2api/nucore/nuvec.hpp"
#include "nu2api/nucore/nuthread.h"
#include "legoapi/legoapi_types.h"

// These renderer objects share the original nu3d initializer and lifetime.
#include "nu2api/nu3d/android/nupostfilter.cpp"

NuDataPortManager NuPostFilterGen::resourceManager;
NuDataPort<nuframebuffer_s *> NuPostFilterGen::portOutFramebuffer;
NuDataPort<NuProxyAttachment *> NuPostFilterGen::portColorBuffer;
NuDataPort<NuProxyAttachment *> NuPostFilterGen::portNormalBuffer;
NuDataPort<NuProxyAttachment *> NuPostFilterGen::portVelocityBuffer;
NuDataPort<NuProxyAttachment *> NuPostFilterGen::portDepthRTBuffer;
NuDataPort<NuProxyAttachment *> NuPostFilterGen::portDepthBuffer;

OcclusionManager g_OcclusionManager;

static const VuVec unitCube[8] = {
    {-1, -1, -1, 1}, {-1, 1, -1, 1}, {1, 1, -1, 1}, {1, -1, -1, 1},
    {-1, -1, 1, 1},  {-1, 1, 1, 1},  {1, 1, 1, 1},  {1, -1, 1, 1},
};
static const VuVec halfUnitCube[8] = {
    {-1, -1, 0, 1}, {-1, 1, 0, 1}, {1, 1, 0, 1}, {1, -1, 0, 1},
    {-1, -1, 1, 1}, {-1, 1, 1, 1}, {1, 1, 1, 1}, {1, -1, 1, 1},
};

template <typename T> struct LightObjectPool {
    struct __attribute__((aligned(16))) Slot {
        // Construction is explicit; reserving the pool must not construct every light.
        u8 storage[sizeof(T)];
    } slots[8];
    i8 occupied;
    i32 next;

    LightObjectPool() : occupied(0), next(0) {
    }

    T *allocate() {
        i32 i;
        for (i = next; i < 8; ++i) {
            if ((occupied & (1 << (i & 7))) == 0) {
                T *result = reinterpret_cast<T *>(slots[i].storage);
                occupied |= 1 << (i & 7);
                next = (i + 1) % 8;
                return result;
            }
        }
        for (i = 0; i < next; ++i) {
            if ((occupied & (1 << (i & 7))) == 0) {
                T *result = reinterpret_cast<T *>(slots[i].storage);
                occupied |= 1 << (i & 7);
                next = (i + 1) % 8;
                return result;
            }
        }
        return NULL;
    }
};
static LightObjectPool<NuDynamicLight> dynamicLightPool;
static LightObjectPool<NURNDRSTATE> rndrStatePool;
static LightObjectPool<NUDISPLAYLISTITEM> dlistItemPool;
DECOMP_ASSERT(sizeof(dynamicLightPool) == 16144, "Dynamic light pool size");
DECOMP_ASSERT(sizeof(dlistItemPool) == 144, "Dynamic light list item pool size");
DECOMP_ASSERT(sizeof(rndrStatePool) == 528, "Dynamic light render state pool size");

const NuVec NuVec::vnull(0, 0, 0);
const NuVec NuVec::vx(1, 0, 0);
const NuVec NuVec::vy(0, 1, 0);
const NuVec NuVec::vz(0, 0, 1);

NuCriticalSection criticalSection("Texture");

NuDynamicLight::RenderSet::RenderSet() {
    parameter_100 = 0.01f;
    geometry_count = 0;
    shadow_plane_count = 0;
    parameter_104 = 0.001f;
    warp_factor = 1.0f;
    for (i32 i = 0; i < 2; ++i) {
        list_items[i] = dlistItemPool.allocate();
        render_states[i] = rndrStatePool.allocate();
        memset(&display_lists[i], 0, sizeof(display_lists[i]));
        NUDISPLAYLISTITEM *item = list_items[i];
        display_lists[i].first = item;
        item->type = 0x8d;
        item->next = NULL;
        item->id = 1;
        display_lists[i].mtl_last = display_lists[i].first;
        display_lists[i].state = render_states[i];
        NuDisplayListReset(&display_lists[i]);
    }
}
NuDynamicLight::NuDynamicLight() {
    reserved_7bc = 0;
    render_set_capacity = 2;
    active_render_set_count = 0;
    parameter_4 = 0;
    parameter_5 = 0;
    used_on_specials = 0;
    reserved_7c0[1] = 4.0f;
    reserved_7c8.x = 0.25f;
    reserved_7c0[0] = 0.8f;
    reserved_7c8.y = 0.0f;
    reserved_7c8.z = 100.0f;
    reserved_7c8.w = 110.0f;
}

NuDynamicLight *NuDynamicLight::create() {
    NuDynamicLight *light = dynamicLightPool.allocate();
    new (light) NuDynamicLight;
    return light;
}

void NuDynamicLight::destroy(NuDynamicLight *light) {
    i32 index = (reinterpret_cast<u8 *>(light) - reinterpret_cast<u8 *>(dynamicLightPool.slots)) /
                i32(sizeof(dynamicLightPool.slots[0]));
    dynamicLightPool.next = index;
    dynamicLightPool.occupied &= ~(1 << (index & 7));
}

#include "nu2api/nu3d/nudynamiclight.cpp"

#include "nu2api/nu3d/nupostfilter_generic.cpp"
