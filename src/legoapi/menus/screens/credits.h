#pragma once

#include "decomp.h"

struct WORLDINFO_s;
struct MENU_s;

struct CREDIT_s {
    char *text;
    f32 x;
    f32 y;
    f32 scale;
    u8 red, green, blue, alpha;
    u8 alignment;
    i8 type;
    u8 reserved[2];
};
DECOMP_ASSERT(sizeof(CREDIT_s) == 0x18, "credit entry ABI");
DECOMP_ASSERT(offsetof(CREDIT_s, type) == 0x15, "credit type offset");

extern CREDIT_s *CreditList;
void Credits_Load(WORLDINFO_s *world, VARIPTR *buffer, VARIPTR *buffer_end);
void Credits_Init(WORLDINFO_s *world);
void Credits_DrawPanel(WORLDINFO_s *world);
void Credits_UpdateMenu(MENU_s *menu);
void Credits_GetInfo(f32 *duration, i32 *flag, f32 *alpha);
