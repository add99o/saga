#include "nu2api/numath/nutrig.h"
#include "nu2api/nucore/nuvuvec.hpp"

f32 NuTrigTable[NUTRIGTABLE_COUNT];

void NuTrigInit(void) {
    for (i32 i = 0; (unsigned char)(i <= NUTRIGTABLE_COUNT - 1); i++) {
        NuTrigTable[i] = sin(i * NUTRIGTABLE_INTERVAL);
    }
}
