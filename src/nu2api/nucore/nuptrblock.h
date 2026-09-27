#pragma once

#include "nu2api/nufile/nufile.h"

#ifdef __cplusplus
extern "C" {
#endif
    void *NuPtrBlockRead(NUFILE file);
    void *NuPtrBlockFix(void *block);
#ifdef __cplusplus
}
#endif
