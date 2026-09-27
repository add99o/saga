#pragma once

#include "nu2api/nucore/common.h"

f32 GetAspectRatio();
void InitStillRender(VARIPTR *buffer, VARIPTR buffer_end);
void NeedScreenGrab(i32 needed);
void GrabStillScreen(void);
void DrawStillScreen(i32 clear);
void DrawPauseScreenWipe(void);
void HandleStillRender(void);
