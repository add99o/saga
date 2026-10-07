#pragma once

#include "nu2api/numath/nufloat.h"

static i16 AISysPathIntersectionAngle(f32 value) {
    f32 absolute = NuFabs(value);
    f32 root = NuFsqrt(1.0f - value * value);
    f32 small = root < absolute ? root : absolute;
    f32 side = (absolute - 0.70710677f) * 3.40282e+38f;
    side = side < 1.0f ? (side > -1.0f ? side : -1.0f) : 1.0f;
    f32 sign = value * 3.40282e+38f;
    sign = sign < 1.0f ? (sign > -1.0f ? sign : -1.0f) : 1.0f;
    f32 product = side * sign;
    f32 x = small * product;
    f32 x2 = x * x;
    f32 x3 = x * x2;
    f32 x4 = x2 * x2;
    f32 x5 = x2 * x3;
    return static_cast<i16>(static_cast<i32>(((sign + product) * 0.785398f - x + (x * -0.166667f) * x2 +
                                              (-0.075f * x2) * x3 + (-0.0446429f * x3) * x4 + (x4 * -0.0303819f) * x5) *
                                             10430.4f));
}
