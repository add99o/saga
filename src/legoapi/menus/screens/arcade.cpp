#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/screens/arcade.h"
#include "gameapi/gui/apimenu.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/menus/core/panel.h"
#include "legoapi/misc.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;
struct MENU_s;

static f32 Arcade_NeedTwoPlayers_Scale = 1.0f;
i32 Arcade_Score[2];

void Arcade_AIKilled(i32 player);
void Arcade_PlayerKilled(i32 player, i32 reason);
void Arcade_AwardPoint(i32 player, i32 reset, i32 reason);
i32 Arcade_BothPlayersActive();
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
extern i16 tWINNER;
extern i16 tARCADE_NEEDTWOPLAYERS;
extern i32 MenuStopDraw;
extern "C" void SmartTextEx(char *, f32, f32, f32, f32, f32, f32, u32, u8, u8, u8, f32, i32, void *, i32, u32);
extern i32 Area;
void SetBonusWinner(i32);
void NewMenu(i32, i32, i32);
void ReCalculateCompletionPoints();
extern "C" i32 TriggerAutoSave();

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

void Arcade_AIKilled(i32 player) {
    if (static_cast<u32>(player) > 1 || (Arcade_Mode[ArcadeItem.field_c_0xc].field8_0x8 & 2) == 0) {
        return;
    }
    if (Arcade_BothPlayersActive() != 0) {
        i32 &kills = (&AreaGlobals.values.field_0x2c)[player];
        ++kills;
        PlaySfx(const_cast<char *>("env_padLight_on"), NULL);
        if (Player[player] != NULL && Player[player]->coinpacket != NULL) {
            Player[player]->coinpacket->scale = 1.5f;
        }
        if (static_cast<u32>(kills) < static_cast<u32>(Arcade_Mode[ArcadeItem.field_c_0xc].target)) {
            return;
        }
        Arcade_AwardPoint(player, 0, 0);
        AreaGlobals.values.field_0x2c = 0;
        AreaGlobals.values.field_0x30 = 0;
        GameObject_s *other = Player[(player ^ 1) & 1];
        if (other != NULL && other->coinpacket != NULL) {
            other->coinpacket->scale = 1.5f;
        }
    } else {
        GameAudio_PlaySfx(0x32, NULL, 0, 0);
        if (Player[player] != NULL && Player[player]->coinpacket != NULL) {
            Player[player]->coinpacket->scale = 1.5f;
        }
        Arcade_NeedTwoPlayers_Scale = 1.5f;
    }
}

void Arcade_DrawPanel(i32 state) {
    if (Arcade == 0 || state != 0 || Arcade_BothPlayersActive() != 0) {
        return;
    }
    const f32 phase = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f) * 2.0f;
    const f32 wave = NuTrigTable[(static_cast<i32>(phase * 65536.0f) >> 1) & 0x7fff];
    const u32 alpha = static_cast<i32>((wave * 0.2f + 0.8f) * 128.0f);
    const f32 text_scale = Arcade_NeedTwoPlayers_Scale * 0.6f;
    const f32 spacing = Arcade_NeedTwoPlayers_Scale * 1.7f;
    SmartTextEx(TTab[tARCADE_NEEDTWOPLAYERS], 0.0f, -0.55f, 1.0f,
                text_scale, text_scale, text_scale, 0, 255, 0, 0,
                spacing, 1, NULL, 0, alpha);
}

void Arcade_AwardPoint(i32 player, i32 reset, i32) {
    if (reset != 0) {
        Arcade_Score[player] = 0;
    } else {
        if (BonusWinner != -1) {
            return;
        }
        if (Arcade_BothPlayersActive() != 0 && ++Arcade_Points[player] > 0) {
            SetBonusWinner(player);
            BonusWinFlag = 0;
            NewMenu(11, -1, -1);
            ++Arcade_Score[player];
        }
    }
    if (Game_LevelSave == NULL) {
        return;
    }
    const i32 mode = Arcade_GetMode(NULL);
    u8 *flags = Game_LevelSave + WORLD->level_idx * 0x54 + 0x53;
    const u8 mask = static_cast<u8>(1 << mode);
    if ((*flags & mask) != 0) {
        return;
    }
    *flags |= mask;
    if (Game_AreaSave != NULL && Area != -1 && (*flags & 7) == 7) {
        Game_AreaSave[Area].area_complete = 1;
        ReCalculateCompletionPoints();
    }
    TriggerAutoSave();
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

void Arcade_CoinCollected(i32 player, u32 *coins, u32 value) {
    if (static_cast<u32>(player) > 1) {
        return;
    }
    const ARCADE_MODE_s &mode = Arcade_Mode[ArcadeItem.field_c_0xc];
    if ((mode.field8_0x8 & 8) != 0) {
        GameObject_s *object = Player[player];
        if (object != NULL && object->coinpacket != NULL &&
            object->coinpacket->coins >= arcade_placed_stud_total) {
            Arcade_AwardPoint(player, 0, 0);
        }
    } else if ((mode.field8_0x8 & 4) != 0) {
        if (Arcade_BothPlayersActive() != 0) {
            GameObject_s *object = Player[player];
            if (object == NULL || object->coinpacket == NULL || object->coinpacket->coins < mode.target) {
                return;
            }
            Arcade_AwardPoint(player, 0, 0);
            if (Player[0] != NULL && Player[0]->coinpacket != NULL) {
                Player[0]->coinpacket->coins = 0;
            }
            if (Player[1] != NULL && Player[1]->coinpacket != NULL) {
                Player[1]->coinpacket->coins = 0;
            }
            GameObject_s *other = Player[(player ^ 1) & 1];
            if (other != NULL && other->coinpacket != NULL) {
                other->coinpacket->scale = 1.5f;
            }
        } else {
            Arcade_NeedTwoPlayers_Scale = 1.25f;
            *coins = value;
        }
    }
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
