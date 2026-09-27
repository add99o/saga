#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/render/core/render.h"
#include "legoapi/menus/core/text.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "globals.h"
#include <stdio.h>

f32 CreditsAlpha;
f32 CreditsTime;
f32 CreditsFinishedTime;
i32 CreditsFlag;
static f32 Credits_Duration = 120.0f;

struct CREDITLINE_s {
    char *text;
    f32 x;
    f32 scroll_y;
    f32 scale;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    u8 alignment;
    i8 type;
    u8 padding[2];
};
DECOMP_ASSERT(sizeof(CREDITLINE_s) == 0x18, "Credit line ABI");

CREDITLINE_s *CreditList;
static volatile f32 CreditsSize;
static volatile i32 CreditCount;
static const f32 CreditWidths[] = {1.9f, 1.9f, 0.935f, 0.935f, 1.9f};

void Credits_Init(WORLDINFO_s *) {
    CreditsAlpha = 1.0f;
    CreditsTime = 0.0f;
    CreditsFinishedTime = 0.0f;
    CreditsFlag = 0;
    if (LastLData != STATUS_LDATA) {
        BackDrop_ResetColours();
    }
    MechSystems::Get()->HookUpClickToPressStart();
}

void Credits_Load(WORLDINFO_s *, variptr_u *, variptr_u *) {
    STUBBED();
}

void Credits_GetInfo(float *duration, i32 *flag, float *alpha) {
    if (duration != NULL) {
        *duration = Credits_Duration;
    }
    if (flag != NULL) {
        *flag = CreditsFlag;
    }
    if (alpha != NULL) {
        *alpha = CreditsAlpha;
    }
}

void __attribute__((optimize("O2,omit-frame-pointer"))) Credits_DrawPanel(WORLDINFO_s *) {
    if (CreditsFlag == 0) {
        const f32 scroll = CreditsTime / Credits_Duration * CreditsSize;
        for (i32 index = 0; index < CreditCount; ++index) {
            const CREDITLINE_s &line = CreditList[index];
            if (scroll < line.scroll_y - 0.2f || scroll > line.scroll_y + 2.2f)
                continue;
            const f32 y = -1.0f - (line.scroll_y - scroll);
            const i32 alpha = static_cast<i32>(static_cast<f32>(line.alpha) * CreditsAlpha);
            SmartTextEx(line.text, line.x, y, 1.0f, line.scale, line.scale, line.scale, line.alignment, line.red,
                        line.green, line.blue, CreditWidths[line.type], 1, NULL, 0, alpha);
        }
        return;
    }

    const f32 completion = Game_CompletionSave != NULL ? static_cast<f32>(*Game_CompletionSave * 100) : 0.0f;
    char text[64];
    sprintf(text, "%.1f%%", completion / static_cast<f32>(COMPLETIONPOINTS));
    f32 y = 0.0f;
    if (CreditsFlag == 1)
        y = 1.0f - CreditsAlpha * 0.1f;
    else if (CreditsFlag == 3)
        y = -(1.0f - CreditsAlpha) * 0.1f;
    const i32 alpha = static_cast<i32>(static_cast<f32>(static_cast<i32>(CreditsAlpha * 255.0f)) * 0.25f);
    SmartTextEx(text, 0.0f, y, 1.0f, 1.125f, 1.125f, 1.125f, 0, 255, 191, 0, 1.7f, 1, NULL, 0, alpha);
}

void Credits_UpdateMenu(MENU_s *) {
    STUBBED();
}
