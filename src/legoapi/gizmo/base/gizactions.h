#pragma once

#include "decomp.h"

struct GIZACTIONDEFN_s;
struct AISYS_s;
struct AISCRIPTPROCESS_s;
struct AIPACKET_s;

i32 Action_EndCameraCut(AISYS_s *system, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params,
                        i32 param_count, i32 first_time, f32 elapsed);
i32 Action_PlayCutScene(AISYS_s *system, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params,
                        i32 param_count, i32 first_time, f32 elapsed);

struct ACTIONINFO_s {
    const char *name;
    u32 flags;
};

struct EXTRAACTIONDATA_s {
    const char *name;
    i32 action;
};

void RegisterGizActions(GIZACTIONDEFN_s *definitions);
void GameRegisterGizActions(void);

extern "C" {
    i32 ActionFromName(const char *name);
    u32 ActionInfoFlags(i32 action);
    const char *ActionInfoName(i32 action);
    void SetActionInfo(ACTIONINFO_s *action_info, EXTRAACTIONDATA_s *extra_action_data);
}
