#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/render/core/render.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/items/objects/gameobjects.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numusic/numusic.h"
#include "gameapi/gui/apimenu.h"
#include "batman.h"
#include "globals.h"
#include <stdio.h>

extern FadeSystem FadeSys;

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
static f32 CreditsSize;
static i32 CreditCount;

struct CREDITTYPE_s {
    const char *name;
    f32 scale;
    f32 width;
    u8 default_colour[4];
    u8 colour[4];
    u8 padding[3];
    u8 type;
};
DECOMP_ASSERT(sizeof(CREDITTYPE_s) == 0x18, "Credit type ABI");

static CREDITTYPE_s CreditType[5] = {
    {"heading", 0.4f, 1.9f, {255, 255, 255, 128}, {}, {}, 0},
    {"company", 0.6f, 1.9f, {255, 255, 255, 128}, {}, {}, 0},
    {"title", 0.35f, 0.935f, {255, 255, 255, 128}, {}, {}, 0},
    {"name", 0.4f, 0.935f, {255, 255, 255, 128}, {}, {}, 0},
    {"credit", 0.4f, 1.9f, {255, 255, 255, 128}, {}, {}, 0},
};

static u8 Credits_ColourComponent(f32 value, f32 maximum) {
    if (value <= 0.0f)
        return 0;
    if (value > 1.0f)
        return static_cast<u8>(maximum);
    return static_cast<u8>(value * maximum);
}

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

void Credits_Load(WORLDINFO_s *, variptr_u *buffer, variptr_u *buffer_end) {
    for (i32 type = 0; type < 5; ++type) {
        for (i32 channel = 0; channel < 4; ++channel)
            CreditType[type].colour[channel] = CreditType[type].default_colour[channel];
        CreditType[type].type = type;
    }

    CreditsSize = 0.0f;
    CreditCount = 0;
    Credits_Duration = 120.0f;

    char filename[256];
    NuStrCpy(filename, "stuff\\text\\");
    NuStrCat(filename, "english");
    NuStrCat(filename, "_credits.txt");

    CreditList = static_cast<CREDITLINE_s *>(GameBufferAlloc(buffer, buffer_end, 1000 * sizeof(CREDITLINE_s)));
    if (CreditList == NULL)
        return;

    NUFPAR *parser = NuFParCreate(filename);
    if (parser == NULL)
        return;

    f32 position = 0.2f;
    while (true) {
        CreditsSize = position + 2.2f;
        if (NuFParGetLine(parser) == 0)
            break;
        if (NuFParGetWord(parser) == 0)
            continue;

        if (NuStrICmp(parser->word_buf, "duration") == 0) {
            if (NuFParGetWord(parser) != 0 && Credits_Duration == 120.0f) {
                const f32 duration = NuAToF(parser->word_buf);
                if (duration > 0.0f)
                    Credits_Duration = duration;
            }
            continue;
        }

        i32 type = 0;
        while (type < 5 && NuStrICmp(CreditType[type].name, parser->word_buf) != 0)
            ++type;
        if (type == 5)
            continue;

        const i32 has_word = NuFParGetWord(parser);
        if (has_word != 0 && NuStrICmp(parser->word_buf, "colour") == 0) {
            for (i32 channel = 0; channel < 4; ++channel) {
                if (NuFParGetWord(parser) != 0) {
                    const f32 maximum = channel == 3 ? 128.0f : 255.0f;
                    CreditType[type].colour[channel] = Credits_ColourComponent(NuAToF(parser->word_buf), maximum);
                }
            }
            continue;
        }

        if (CreditCount >= 1000)
            continue;

        CREDITLINE_s &line = CreditList[CreditCount++];
        line.text = NULL;
        if (has_word != 0 && parser->word_buf[0] != '\0') {
            line.text = static_cast<char *>(GameBufferAlloc(buffer, buffer_end, NuStrLen(parser->word_buf) + 1));
            if (line.text != NULL)
                NuStrCpy(line.text, parser->word_buf);
        }
        line.x = type == 2 ? -0.015f : type == 3 ? 0.015f : 0.0f;
        line.scroll_y = position;
        line.scale = CreditType[type].scale;
        line.red = CreditType[type].colour[0];
        line.green = CreditType[type].colour[1];
        line.blue = CreditType[type].colour[2];
        line.alpha = CreditType[type].colour[3];
        line.alignment = type == 2 ? 8 : type == 3 ? 2 : 0;
        line.type = CreditType[type].type;
        if (type != 2)
            position += CreditType[type].scale / CreditType[1].scale * 0.2f;
    }
    NuFParDestroy(parser);
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
                        line.green, line.blue, CreditType[line.type].width, 1, NULL, 0, alpha);
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

void Credits_UpdateMenu(MENU_s *menu) {
    const f32 music_volume = Game_OptionsSave != NULL ? GameSetMusicVolume(Game_OptionsSave) : 1.0f;

    if (CreditsFlag == 0) {
        if (FadeSys.fade == 0.0f)
            CreditsTime += FRAMETIME;

        if (CreditsFinishedTime > 0.0f) {
            CreditsFinishedTime += FRAMETIME;
            if (CreditsFinishedTime >= 1.1f) {
                CreditsFlag = 1;
                CreditsAlpha = 0.0f;
                CreditsTime = 0.0f;
            } else if (CreditsAlpha > 0.0f) {
                CreditsAlpha -= FRAMETIME;
                if (CreditsAlpha < 0.0f)
                    CreditsAlpha = 0.0f;
            }
        } else if ((menu->buttons_pressed & GAMEPAD_SKIP) != 0 || CreditsTime >= Credits_Duration ||
                   MechSystems::SkipTextScroll != 0) {
            CreditsFinishedTime = 0.001f;
            MechSystems::Get()->UnhookClickToPressStart();
        }

        legoSetMusicVolume(CreditsAlpha * music_volume);
        return;
    }

    legoSetMusicVolume(0.0f);
    CreditsTime += FRAMETIME;

    if (CreditsFlag == 1) {
        if (FRAMETIME < 0.1f && CreditsTime >= 0.1f) {
            music_man.StopAll(0);
            MusicClearAll();
            SoundKillAll();
        }
        if (CreditsTime >= 1.0f) {
            CreditsFlag = 2;
            GameAudio_PlaySfx(0x2b, NULL, 0, 0);
            CreditsAlpha = NuTrigTable[0x2000];
        } else {
            const i32 angle = static_cast<i32>(CreditsTime * 16384.0f);
            CreditsAlpha = NuTrigTable[(angle >> 1) & 0x7fff];
        }
        return;
    }

    if (CreditsFlag == 2) {
        if (CreditsTime >= 3.0f) {
            CreditsFlag = 3;
            CreditsTime = 0.0f;
        }
        CreditsAlpha = NuTrigTable[0x2000];
        return;
    }

    if (CreditsTime >= 1.1f) {
        NewLData = HUB_LDATA;
        const FADETYPE fade = {FADE_TYPE_STILL};
        FadeSys.SetFade(fade, 0);
    }
    const f32 remaining = 1.0f - CreditsTime;
    if (remaining < 0.0f)
        CreditsAlpha = NuTrigTable[0];
    else {
        const i32 angle = static_cast<i32>(remaining * 16384.0f);
        CreditsAlpha = NuTrigTable[(angle >> 1) & 0x7fff];
    }
}
