#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/screens/arcade.h"
#include "gameapi/gui/apimenu.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/timer.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;
struct MENU_s;

static f32 Arcade_NeedTwoPlayers_Scale = 1.0f;

void Arcade_AIKilled(i32 player);
void Arcade_PlayerKilled(i32 player, i32 reason);
i32 Arcade_BothPlayersActive();
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
extern i16 tWINNER;
extern i32 MenuStopDraw;
extern "C" void SmartTextEx(char *, f32, f32, f32, f32, f32, f32, u32, u8, u8, u8, f32, i32, void *, i32, u32);

void Arcade_Kill(i32 player, i32 killer) {
    if (Arcade != 0 && static_cast<u32>(player) <= 1) {
        if (static_cast<u32>(killer) <= 1) {
            Arcade_PlayerKilled(player, 0);
        } else if (killer == -1) {
            Arcade_AIKilled(player);
        }
    }
}

i32 Arcade_GetMode(u32 *flags) {
    if (Arcade == 0) {
        if (flags != NULL) {
            *flags = 0;
        }
        return -1;
    }
    if (flags != NULL) {
        *flags = Arcade_Mode[ArcadeItem.field_c_0xc].field8_0x8;
    }
    return ArcadeItem.field_c_0xc;
}

void Arcade_AIKilled(i32) {
    STUBBED();
}

void Arcade_DrawPanel(i32) {
    STUBBED();
}

void Arcade_AwardPoint(i32, i32, i32) {
    STUBBED();
}

void Arcade_ResetPanel() {
    Arcade_NeedTwoPlayers_Scale = 1.0f;
}

void Arcade_DrawEndMenu(MENU_s *) {
    if (MenuStopDraw == 0) {
        SmartTextEx(TTab[tWINNER], 0.0f, STATSPOSY, 1.0f, 1.0f, 1.0f, 1.0f, 0, 0,
                    static_cast<u32>(menu_flash) < 1 ? 255 : 191, 0, 1.7f, 1, NULL, 0, 0x80);
    }
}

void Arcade_UpdatePanel(i32 state) {
    if (Arcade == 0 || state != 0 || Arcade_BothPlayersActive() != 0) {
        Arcade_NeedTwoPlayers_Scale = 1.0f;
        return;
    }
    Arcade_NeedTwoPlayers_Scale = SeekLinearF(Arcade_NeedTwoPlayers_Scale, 1.0f, FRAMETIME);
    if (static_cast<i32>(GameTimer.time_elapsed * 2.0f) !=
        static_cast<i32>(GameTimer.last_time_elapsed * 2.0f)) {
        GameAudio_PlaySfx(0x35, NULL, 0, 0);
    }
}

void Arcade_PlayerKilled(i32 player, i32 reason) {
    if (static_cast<u32>(player) > 1 || (Arcade_Mode[ArcadeItem.field_c_0xc].field8_0x8 & 1) == 0) {
        return;
    }
    if (Arcade_BothPlayersActive() != 0) {
        (&AreaGlobals.values.field_0x24)[player] += 1;
        Arcade_AwardPoint(player, 0, reason);
    } else {
        Arcade_AwardPoint(player, 0, 0);
    }
}

void Arcade_CoinCollected(i32, u32 *, u32) {
    STUBBED();
}

void Arcade_UpdateEndMenu(MENU_s *menu) {
    if (menu->menu_time >= 5.0f) {
        NewLData = HUB_LDATA;
    } else if (menu->menu_time >= 1.5f && BonusWinFlag == 0) {
        PlaySfx(const_cast<char *>("Victory"), NULL);
        BonusWinFlag = 1;
    }
}

i32 Arcade_BothPlayersActive() {
    return true;
}
