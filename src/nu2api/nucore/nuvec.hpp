#pragma once

#include "decomp.h"
#include "nu2api/nucore/common.h"

class NuVec {
  public:
    f32 x, y, z;

    NuVec(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {
    }

    static const NuVec vnull, vx, vy, vz;
};
DECOMP_ASSERT(sizeof(NuVec) == 12, "NuVec size");
