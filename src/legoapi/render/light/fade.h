#pragma once

#include "legoapi/legoapi_types.h"
#include "legoapi/render/light/fade_material.h"

void SetFramesToWait(u32 frames);

extern nugscn_s *FadeLoop_ObjScene;
extern nuhspecial_s FadeLoop_ObjHSpecial;
void FadeLoop_SetObj(nugscn_s *scene, char *name);
void FadeLoop_DrawObj(f32 alpha);
i32 FadeLoop_UsingObj();
// Duration/frame times must allow the linear seek to reach its endpoint.
void FadeLoop(char *text, i32 direction, f32 duration, void (*draw)(f32));

inline FADETYPE_VALUE Fade::GetFadeType() const {
    return FADE_TYPE_SCREEN;
}

inline FADETYPE_VALUE FadeWipe::GetFadeType() const {
    return FADE_TYPE_WIPE;
}

inline FADETYPE_VALUE FadeStillWipe::GetFadeType() const {
    return FADE_TYPE_STILL_WIPE;
}

inline FADETYPE_VALUE FadeStill::GetFadeType() const {
    return FADE_TYPE_STILL;
}
