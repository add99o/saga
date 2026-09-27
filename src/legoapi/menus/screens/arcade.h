#pragma once

#include "decomp.h"

struct MENU_s;

extern i32 Arcade_Score[2];
i32 Arcade_BothPlayersActive();
i32 Arcade_GetMode(u32 *flags);
void Arcade_ResetPanel();
void Arcade_UpdatePanel(i32 paused);
void Arcade_DrawPanel(i32 paused);
void Arcade_AwardPoint(i32 player_index, i32 reset_score, i32 extra);
void Arcade_PlayerKilled(i32 player_index, i32 extra);
void Arcade_AIKilled(i32 player_index);
void Arcade_CoinCollected(i32 player_index, u32 *score, u32 previous_score);
void Arcade_Kill(i32 player_index, i32 killed_player);
void Arcade_UpdateEndMenu(MENU_s *menu);
void Arcade_DrawEndMenu(MENU_s *menu);
