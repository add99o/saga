// Pending-transcription stand-ins (original exports without decompiled
// bodies yet). Collected here so each is an explicit, greppable TODO;
// they previously lived as anonymous extern-C stubs that shadowed real
// transcriptions elsewhere.

#include "decomp.h"
#include "globals.h"
#include "nu2api/nu3d/nutexanm.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/ShaderManagerOpenGL.h"
#include "nu2api/nu3d/nushader_plain.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/numath/nurand.h"

extern "C" {
u8 uberShader2_md5[16] = {0x38, 0x2a, 0x9d, 0x15, 0xf8, 0xfa, 0xbf, 0x09,
                          0xcb, 0xcc, 0x9b, 0xec, 0x5e, 0xb7, 0x62, 0x40};
}

extern "C" void NuShaderManagerForceShader(void) {
}

extern "C" void *NuShaderManagerGetInstance(void) {
    return g_shaderManager;
}

extern "C" f32 NuShaderManagerGetShininessFactor(void) {
    return ShaderManagerTemplate<NuShaderObject>::shininessFactor;
}

extern "C" void NuShaderManagerLoadCompiledShaders(void) {
}

extern "C" void NuShaderManagerSetShininessFactor(f32 shininess) {
    ShaderManagerTemplate<NuShaderObject>::shininessFactor = shininess;
}

extern "C" void NuShaderObjectKeyGenerate2(u32 *key, const nushadermtldesc_s *description,
                                            const numtl_s *material, i32 flags, i32 variant, i32 pixel_stage) {
    ShaderMtlDescFilter filter;
    filter.internalInit(description, material, flags, variant);
    NuShaderObjectKeyGenerate3(key, &filter, pixel_stage);
}
