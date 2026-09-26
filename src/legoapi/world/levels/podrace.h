#pragma once

#include "decomp.h"
#include "nu2api/numath/numtx.h"

struct GameObject_s;

struct _vuv_s {
    float x, y, z, w;
};

// One pod in the race state. Shared with the level-state allocator so host
// allocations follow pointer-bearing fields instead of the target byte size.
struct racepod_s {
    NUMTX matrix;
    _vuv_s previous_axis;
    _vuv_s previous_position;
    char pad_0x60[0x10];
    i32 pitch;
    i32 yaw;
    i32 pad_0x78;
    float speed;
    u32 *data;
    float start;
    i16 model_id;
    i16 pad_0x8a;
    float distance;
    GameObject_s *object;
    void *next;
};
using PODRACE_LAPENTRY_s = racepod_s;
DECOMP_ASSERT(sizeof(racepod_s) == 0x98, "racepod layout");

struct PODRACE_s {
    char pad_0x0000[0xa580];
    PODRACE_LAPENTRY_s lap_entries[0x10];
    float lap_countdown;
    float mushroom_timer;
    float lap_display;
    float prev_lap_display;
    float max_lap_time;
    float lap_time_increment;
    i32 lap_attempts_per_increment;
    char pad_0xaf1c[0xaf20 - 0xaf1c];
    u8 flags;
    char pad_0xaf21[0xaf24 - 0xaf21];
};
DECOMP_ASSERT(offsetof(PODRACE_s, lap_entries) == 0xa580, "podrace lap entries offset");
DECOMP_ASSERT(sizeof(PODRACE_s) == 0xaf24, "podrace state size");
