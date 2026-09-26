#include "decomp.h"
#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "globals.h"
#include "legoapi/audio/audio.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/panel.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/menus/screens/arcade.h"
#include "legoapi/menus/screens/gamestatus_lsw.h"
#include "legoapi/misc.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

extern i16 tWINNER;
extern i16 tARCADE_NEEDTWOPLAYERS;

static f32 Arcade_NeedTwoPlayers_Scale = 1.0f;
i32 Arcade_Score[2];

i32 Arcade_BothPlayersActive() {
    // The shipped mobile implementation returns true unconditionally.
    return true;
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

void Arcade_ResetPanel() {
    Arcade_NeedTwoPlayers_Scale = 1.0f;
}

void Arcade_UpdatePanel(i32 paused) {
    if (Arcade == 0 || paused != 0 || Arcade_BothPlayersActive()) {
        Arcade_NeedTwoPlayers_Scale = 1.0f;
    } else {
        Arcade_NeedTwoPlayers_Scale = SeekLinearF(Arcade_NeedTwoPlayers_Scale, 1.0f, FRAMETIME);
        if (static_cast<i32>(GameTimer.time_elapsed * 2.0f) != static_cast<i32>(GameTimer.last_time_elapsed * 2.0f)) {
            GameAudio_PlaySfx(0x35, NULL, 0, 0);
        }
    }
}

void Arcade_DrawPanel(i32 paused) {
    if (Arcade == 0 || paused != 0 || Arcade_BothPlayersActive()) {
        return;
    }
    f32 phase = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f);
    i32 angle = static_cast<i32>((phase + phase) * 65536.0f);
    i32 alpha = static_cast<i32>((NuTrigTable[(angle >> 1) & 0x7fff] * 0.2f + 0.8f) * 128.0f);
    f32 scale = Arcade_NeedTwoPlayers_Scale * 0.6f;
    SmartTextEx(TTab[tARCADE_NEEDTWOPLAYERS], 0.0f, -0.55f, 1.0f, scale, scale, scale, 0, 255, 0, 0,
                Arcade_NeedTwoPlayers_Scale * 1.7f, 1, NULL, 0, alpha);
}

void Arcade_AwardPoint(i32 player_index, i32 reset_score, i32) {
    if (reset_score != 0) {
        Arcade_Score[player_index] = 0;
    } else {
        if (BonusWinner != -1) {
            return;
        }
        if (Arcade_BothPlayersActive()) {
            ++Arcade_Points[player_index];
            if (Arcade_Points[player_index] > 0) {
                SetBonusWinner(player_index);
                BonusWinFlag = 0;
                NewMenu(0xb, -1, -1);
                ++Arcade_Score[player_index];
            }
        }
    }
    if (Game_LevelSave != NULL) {
        u8 mode_bit = 1u << Arcade_GetMode(NULL);
        LEVELSAVE_s *saves = reinterpret_cast<LEVELSAVE_s *>(Game_LevelSave);
        LEVELSAVE_s *save = &saves[WORLD->level_idx];
        if ((save->arcade_flags & mode_bit) == 0) {
            save->arcade_flags |= mode_bit;
            if (Game_AreaSave != NULL && Area != -1 && (saves[WORLD->level_idx].arcade_flags & 7) == 7) {
                Game_AreaSave[Area].area_complete = 1;
                ReCalculateCompletionPoints();
            }
            TriggerAutoSave();
        }
    }
}

void Arcade_PlayerKilled(i32 player_index, i32 extra) {
    if (static_cast<u32>(player_index) > 1 || (Arcade_Mode[ArcadeItem.field_c_0xc].field8_0x8 & 1) == 0) {
        return;
    }
    if (Arcade_BothPlayersActive()) {
        ++AreaGlobals.values.arcade_player_kills[player_index];
        Arcade_AwardPoint(player_index, 0, extra);
    } else {
        Arcade_AwardPoint(player_index, 0, 0);
    }
}

void Arcade_AIKilled(i32 player_index) {
    if (static_cast<u32>(player_index) > 1 || (Arcade_Mode[ArcadeItem.field_c_0xc].field8_0x8 & 2) == 0) {
        return;
    }
    if (Arcade_BothPlayersActive()) {
        ++AreaGlobals.values.arcade_ai_kills[player_index];
        PlaySfx(const_cast<char *>("env_padLight_on"), NULL);
        if (Player[player_index] != NULL && Player[player_index]->coinpacket != NULL) {
            Player[player_index]->coinpacket->scale = 1.5f;
        }
        if (AreaGlobals.values.arcade_ai_kills[player_index] >=
            static_cast<u32>(Arcade_Mode[ArcadeItem.field_c_0xc].target)) {
            Arcade_AwardPoint(player_index, 0, 0);
            AreaGlobals.values.arcade_ai_kills[0] = 0;
            AreaGlobals.values.arcade_ai_kills[1] = 0;
            GameObject_s *other = Player[(player_index + 1) & 1];
            if (other != NULL && other->coinpacket != NULL) {
                other->coinpacket->scale = 1.5f;
            }
        }
    } else {
        GameAudio_PlaySfx(0x32, NULL, 0, 0);
        if (Player[player_index] != NULL && Player[player_index]->coinpacket != NULL) {
            Player[player_index]->coinpacket->scale = 1.5f;
        }
        Arcade_NeedTwoPlayers_Scale = 1.5f;
    }
}

void Arcade_CoinCollected(i32 player_index, u32 *score, u32 previous_score) {
    if (static_cast<u32>(player_index) > 1) {
        return;
    }
    i32 flags = Arcade_Mode[ArcadeItem.field_c_0xc].field8_0x8;
    if ((flags & 8) != 0) {
        if (Player[player_index] != NULL && Player[player_index]->coinpacket != NULL &&
            Player[player_index]->coinpacket->coins >= arcade_placed_stud_total) {
            Arcade_AwardPoint(player_index, 0, 0);
        }
    } else if ((flags & 4) != 0) {
        if (Arcade_BothPlayersActive()) {
            if (Player[player_index] != NULL && Player[player_index]->coinpacket != NULL &&
                Player[player_index]->coinpacket->coins >=
                    static_cast<u32>(Arcade_Mode[ArcadeItem.field_c_0xc].target)) {
                Arcade_AwardPoint(player_index, 0, 0);
                if (Player[0] != NULL && Player[0]->coinpacket != NULL) {
                    Player[0]->coinpacket->coins = 0;
                }
                if (Player[1] != NULL && Player[1]->coinpacket != NULL) {
                    Player[1]->coinpacket->coins = 0;
                }
                GameObject_s *other = Player[(player_index + 1) & 1];
                if (other != NULL && other->coinpacket != NULL) {
                    other->coinpacket->scale = 1.5f;
                }
            }
        } else {
            *score = previous_score;
            Arcade_NeedTwoPlayers_Scale = 1.25f;
        }
    }
}

void Arcade_Kill(i32 player_index, i32 killed_player) {
    if (Arcade != 0 && static_cast<u32>(player_index) <= 1) {
        if (static_cast<u32>(killed_player) <= 1) {
            Arcade_PlayerKilled(player_index, 0);
        } else if (killed_player == -1) {
            Arcade_AIKilled(player_index);
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

void Arcade_DrawEndMenu(MENU_s *) {
    if (MenuStopDraw != 0) {
        return;
    }
    SmartTextEx(TTab[tWINNER], 0.0f, STATSPOSY, 1.0f, 1.0f, 1.0f, 1.0f, 0, 0, menu_flash ? 191 : 255, 0, 1.7f, 1, NULL,
                0, 128);
}
