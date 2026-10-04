#pragma once

#include "decomp.h"
#include "nu2api/numath/nuvec.h"

class VuVec {
  public:
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
        };
        NUVEC xyz;
    };
    f32 w;

    VuVec() = default;

    VuVec(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {
    }
};
DECOMP_ASSERT(sizeof(VuVec) == 0x10, "VuVec size");
DECOMP_ASSERT(offsetof(VuVec, xyz) == 0, "VuVec three-dimensional view offset");
DECOMP_ASSERT(offsetof(VuVec, w) == 0xc, "VuVec fourth component offset");
