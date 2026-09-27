#pragma once

#include "decomp.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nuvec.h"

struct rtl_s;

// Both original ABI tags share this runtime layout. Pointer-bearing storage
// grows naturally on diagnostic hosts instead of retaining Android byte offsets.
struct rtlidata_s {
    union {
        u8 data[0x144];
        struct {
            rtl_s *directional_lights[3];
            f32 directional_strengths[3];
            rtl_s *ambient_lights[3];
            f32 ambient_strengths[3];
            rtl_s *anti_lights[3];
            f32 anti_strengths[3];
            i32 anti_light_count;
            rtl_s *cached_light;
            NUVEC shadow_direction;
            f32 cached_value;
            NUVEC previous_shadow_direction;
            f32 previous_shadow_value;
            f32 shadow_blend;
            u16 cached_light_uid;
            u8 reserved_76[2];
            union {
                NUCOLOUR3 intensity[3];
                NUVEC intensity_vectors[3];
            };
            NUVEC direction[3];
            union {
                NUVEC ambient;
                NUCOLOUR3 ambient_colour;
            };
            u8 reserved_cc[0x54];
            f32 field_120;
            NUVEC blended_shadow_direction;
            f32 blended_shadow_value;
            NUVEC field_134;
            f32 specular_value;
        };
    };
};

struct rtldata_s : rtlidata_s {};

DECOMP_ASSERT(sizeof(rtlidata_s) == 0x144, "rtlidata_s size");
DECOMP_ASSERT(sizeof(rtldata_s) == 0x144, "rtldata_s size");
DECOMP_ASSERT(offsetof(rtlidata_s, anti_light_count) == 0x48, "RTL anti-light count offset");
DECOMP_ASSERT(offsetof(rtlidata_s, cached_light) == 0x4c, "RTL cached light offset");
DECOMP_ASSERT(offsetof(rtlidata_s, cached_light_uid) == 0x74, "RTL cached UID offset");
DECOMP_ASSERT(offsetof(rtlidata_s, intensity) == 0x78, "RTL intensity offset");
DECOMP_ASSERT(offsetof(rtlidata_s, direction) == 0x9c, "RTL direction offset");
DECOMP_ASSERT(offsetof(rtlidata_s, ambient) == 0xc0, "RTL ambient offset");
DECOMP_ASSERT(offsetof(rtlidata_s, blended_shadow_direction) == 0x124, "RTL blended shadow offset");
