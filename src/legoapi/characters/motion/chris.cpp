#include "decomp.h"
#include "globals.h"
#include "legoapi/items/collect/spacelevel.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/props/system/socksys.h"
#include "legoapi/render/core/terrain.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuhspecial.h"
#include "nu2api/nu3d/nuspecial.h"
#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern f32 SpaceRumbleTimer;
extern spacelevel_scale_s STARFIGHTERDRAWSCALE;
extern GameObject_s *Player[8];
extern f32 FRAMETIME;
extern LEVELDATA_s *DOGFIGHTA_LDATA;
extern BOLT_s Bolt[32];
extern i32 i_bolt;
extern f32 BOLT_OVERRIDE_PLAYERBOLTSPEED;
extern f32 BOLT_OVERRIDE_PLAYERBOLTDURATION;
// Four debug door keys start enabled and are restored by each restart.
i32 DogDebKey[4] __attribute__((aligned(16))) = {-1, -1, -1, -1};
struct quickboltinfo;
extern "C" void NuSpecialList(NUGSCN *);
extern "C" i32 NuSpecialFind(NUGSCN *, nuhspecial_s *, char *, i32);
extern "C" i32 NuSpecialExistsFn(void *);
extern "C" nuvec_s *NuSpecialGetPos(void *);
void ChrisAnakinCReset();
static NUVEC4 RadialMoveCentre;
static __used__ f32 RadialPlayerRadius[2];
static __used__ f32 MaxRadialCamY;

void ResetSpaceLevel(WORLDINFO_s *, spacelevel_s *) __asm__("_ZL15ResetSpaceLevelP11WORLDINFO_sP12spacelevel_s")
    __attribute__((visibility("hidden")));
void ResetSpaceLevel(WORLDINFO_s *world, spacelevel_s *space) {
    i32 door_index;
    space->unknown_62eb8 = 0;
    space->player_origin = {-1456.9f, 326.5f, -394.0f};
    space->player_origin_padding = 0.0f;
    space->direction = {1447.4901f, -492.25f, -704.0f};
    space->camera_origin = {-9.409912f, -165.75f, -1098.0f};

    space->direction_length = NuVecMag(&space->direction);
    space->inverse_direction_length = 1.0f / space->direction_length;

#define DOOR_REACHED(player_index, door_index)                                                                         \
    (Player[player_index]->field_0x68c > DogFightDoors.doors[door_index].distance)
    if (Player[0] != NULL) {
        if (DOOR_REACHED(0, 6)) {
            goto door_6;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 6)) {
            goto door_6;
        }
        if (DOOR_REACHED(0, 5)) {
            goto door_5;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 5)) {
            goto door_5;
        }
        if (DOOR_REACHED(0, 4)) {
            goto door_4;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 4)) {
            goto door_4;
        }
        if (DOOR_REACHED(0, 3)) {
            goto door_3;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 3)) {
            goto door_3;
        }
        if (DOOR_REACHED(0, 2)) {
            goto door_2;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 2)) {
            goto door_2;
        }
        if (DOOR_REACHED(0, 1)) {
            goto door_1;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 1)) {
            goto door_1;
        }
        if (DOOR_REACHED(0, 0)) {
            goto door_0;
        }
        if (Player[1] != NULL && DOOR_REACHED(1, 0)) {
            goto door_0;
        }
    } else if (Player[1] != NULL) {
        if (DOOR_REACHED(1, 6)) {
            goto door_6;
        }
        if (DOOR_REACHED(1, 5)) {
            goto door_5;
        }
        if (DOOR_REACHED(1, 4)) {
            goto door_4;
        }
        if (DOOR_REACHED(1, 3)) {
            goto door_3;
        }
        if (DOOR_REACHED(1, 2)) {
            goto door_2;
        }
        if (DOOR_REACHED(1, 1)) {
            goto door_1;
        }
        if (DOOR_REACHED(1, 0)) {
            goto door_0;
        }
    }

#undef DOOR_REACHED

    space->door_countdown = 0.0f;
    space->door_time = 0.0f;
    space->door_elapsed = 0.0f;

reset_space:

#define RESET_STARFIGHTER(fighter)                                                                                     \
    fighter.reset_timer = 0;                                                                                           \
    fighter.reset_state = 0;                                                                                           \
    fighter.reset_target = 0
#define RESET_TROOPER_TEAM(team)                                                                                       \
    team.reset_timer = 0;                                                                                              \
    team.reset_state = 0;                                                                                              \
    team.reset_target = 0;                                                                                             \
    team.reset_colour = 0xff00;                                                                                        \
    team.reset_effect = 0;                                                                                             \
    team.reset_effect_timer = 0

    RESET_STARFIGHTER(space->fighter_groups[0].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[0].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[0].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[0].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[0].trooper_team);
    RESET_STARFIGHTER(space->fighter_groups[1].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[1].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[1].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[1].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[1].trooper_team);
    RESET_STARFIGHTER(space->fighter_groups[2].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[2].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[2].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[2].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[2].trooper_team);
    RESET_STARFIGHTER(space->fighter_groups[3].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[3].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[3].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[3].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[3].trooper_team);
    RESET_STARFIGHTER(space->fighter_groups[4].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[4].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[4].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[4].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[4].trooper_team);
    RESET_STARFIGHTER(space->fighter_groups[5].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[5].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[5].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[5].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[5].trooper_team);
    RESET_STARFIGHTER(space->fighter_groups[6].fighters[0]);
    RESET_STARFIGHTER(space->fighter_groups[6].fighters[1]);
    RESET_STARFIGHTER(space->fighter_groups[6].fighters[2]);
    RESET_STARFIGHTER(space->fighter_groups[6].fighters[3]);
    RESET_TROOPER_TEAM(space->fighter_groups[6].trooper_team);
    RESET_STARFIGHTER(space->final_fighters[0]);
    RESET_STARFIGHTER(space->final_fighters[1]);
    RESET_STARFIGHTER(space->final_fighters[2]);
    RESET_STARFIGHTER(space->final_fighters[3]);
    space->last_starfighter.reset_timer = 0;
    space->last_starfighter.reset_state = 0;
    space->last_starfighter.reset_target = 0;
    space->last_starfighter.reset_colour = 0xff00;
    space->last_starfighter.reset_effect = 0;
    space->last_starfighter.reset_effect_timer = 0;

#undef RESET_TROOPER_TEAM
#undef RESET_STARFIGHTER

    if (world->current_level == DOGFIGHTA_LDATA) {
        space->unknown_3370 = NULL;
    } else {
        space->unknown_3370 = &Actions_AnakinA;
    }
    if (space->reset_buffer_count != 0) {
        memset(space->reset_buffer, 0, space->reset_buffer_count * 96);
        space->reset_buffer_used = 0;
    }
    space->unknown_337c = 4;
    space->unknown_3374 = 0;
    space->unknown_3378 = 0;
    space->unknown_338c = space;
    space->value_one_a = 1.0f;
    space->value_one_b = 1.0f;

    space->player_matrix_flags = 0;
    space->player_colour = 0xffffff;
    space->player_matrix.m00 = 80.0f;
    space->player_matrix.m01 = 55.0f;
    space->player_matrix.m02 = 200.0f;
    space->player_matrix.m03 = 1.0f;
    space->player_matrix.m30 = 0.0f;
    space->player_matrix.m31 = 0.0f;
    space->player_matrix.m32 = 0.0f;
    space->player_matrix.m33 = 1.0f;
    space->player_matrix.m10 = 0.0f;
    space->player_matrix.m11 = 0.0f;
    space->player_matrix.m13 = 1.0f;
    space->player_matrix.m20 = 0.0f;
    space->player_matrix.m21 = 0.0f;
    space->player_matrix.m22 = 0.0f;
    space->player_matrix.m23 = 1.0f;
    space->player_matrix.m12 = 200.0f;
    space->player_matrix_state = 0;

    space->camera_matrix.m00 = 5.0f;
    space->camera_matrix.m01 = 80.0f;
    space->camera_matrix.m02 = 55.0f;
    space->camera_matrix.m03 = 200.0f;
    space->camera_matrix.m30 = 0.0f;
    space->camera_matrix.m31 = 0.0f;
    space->camera_matrix.m32 = 0.0f;
    space->camera_matrix.m33 = 1.0f;
    space->camera_matrix.m10 = 0.0f;
    space->camera_matrix.m11 = 0.0f;
    space->camera_matrix.m13 = 1.0f;
    space->camera_matrix.m20 = 0.0f;
    space->camera_matrix.m21 = 0.0f;
    space->camera_matrix.m22 = 0.0f;
    space->camera_matrix.m23 = 1.0f;
    space->camera_matrix.m12 = 200.0f;
    space->camera_matrix_w = 1.0f;
    space->camera_value = 5.0f;
    space->camera_colour = 0xffffff;
    space->camera_matrix_flags = 0;
    space->camera_matrix_state = 0;

    for (i32 i = 0; i < 96; ++i) {
        space->queued_starfighters[i].reset_timer = 0;
        space->queued_starfighters[i].reset_effect = 0;
        space->queued_starfighters[i].reset_effect_timer = 0;
    }
    for (i32 i = 0; i < 256; ++i) {
        space->large_records[i].saved_value = space->large_records[i].reset_value;
        space->large_records[i].saved_state = space->large_records[i].reset_state;
    }
    SpaceRumbleTimer = NuRandFloat();
    return;

door_6:
    door_index = 6;
    goto set_door_timer;
door_5:
    door_index = 5;
    goto set_door_timer;
door_4:
    door_index = 4;
    goto set_door_timer;
door_3:
    door_index = 3;
    goto set_door_timer;
door_2:
    door_index = 2;
    goto set_door_timer;
door_1:
    door_index = 1;
    goto set_door_timer;
door_0:
    door_index = 0;

set_door_timer:
    space->door_time = DogFightDoors.doors[door_index].timer;
    space->door_countdown = space->door_time * 0.5f / 1000.0f;
    space->door_elapsed = (space->door_countdown - FRAMETIME) * 0.5f / 1000.0f;
    goto reset_space;
}

void ChrisRadialCam(nuvec_s *position, nuvec_s *target) {
    const f32 position_y = position->y;
    const f32 target_y = target->y;
    NUVEC position_delta = {position->x - RadialMoveCentre.x, 0.0f, position->z - RadialMoveCentre.z};
    const f32 position_radius = NuVecMag(&position_delta);
    NUVEC origin_delta = {-RadialMoveCentre.x, 0.0f, -RadialMoveCentre.z};
    const f32 origin_radius = NuVecMag(&origin_delta);
    RadialPlayerRadius[0] = origin_radius;

    f32 base_radius = 120.0f;
    if (origin_radius >= 120.0f) {
        base_radius = MIN(150.0f, origin_radius);
    }
    const f32 extra_radius = origin_radius - base_radius;
    f32 target_radius = base_radius + extra_radius;
    if (position_radius != 0.0f) {
        const f32 scale = target_radius / position_radius;
        position_delta.x *= scale;
        position_delta.z *= scale;
    }
    position->x = RadialMoveCentre.x + position_delta.x;
    position->y = position_y;
    position->z = RadialMoveCentre.z + position_delta.z;

    if (base_radius <= origin_radius) {
        target_radius = base_radius + 0.6f * extra_radius;
    }
    if (origin_radius != 0.0f) {
        const f32 scale = target_radius / origin_radius;
        origin_delta.x *= scale;
        origin_delta.z *= scale;
    }
    target->x = RadialMoveCentre.x + origin_delta.x;
    target->y = target_y;
    target->z = RadialMoveCentre.z + origin_delta.z;
}

void ChrisAnakinAInit(WORLDINFO_s *world) {
    ResetSpaceLevel(world, world->space_level);
}

void ChrisAnakinBDraw() {
}

void ChrisAnakinBInit() {
    RadialMoveCentre.x = 0.0f;
    RadialMoveCentre.y = 0.0f;
    RadialMoveCentre.z = 0.0f;
    RadialMoveCentre.w = 1.0f;
    MaxRadialCamY = 8.36f;
    NuSpecialList(WORLD->current_gscn);
    nuhspecial_s centre;
    if (NuSpecialFind(WORLD->current_gscn, &centre, "Centre", 1) != 0 && NuSpecialExistsFn(&centre) != 0) {
        nuvec_s *position = NuSpecialGetPos(&centre);
        memcpy(&RadialMoveCentre, position, sizeof(NUVEC));
    }
}

void ChrisAnakinCInit() {
    NuSpecialList(WORLD->current_gscn);
    ChrisAnakinCReset();
}

void ChrisAnakinDInit(WORLDINFO_s *world) {
    ResetSpaceLevel(world, world->space_level);
}

void DogFightARestart() {
    memset(DogDebKey, 0xff, sizeof(DogDebKey));
}

void ChrisAnakinAPanel(WORLDINFO_s *) {
}

void ChrisAnakinAReset(WORLDINFO_s *world) {
    ResetSpaceLevel(world, world->space_level);
}

void ChrisAnakinBReset() {
}

struct AnakinCEntry {
    NUMTX draw_matrix[2];
    NUMTX transform_matrix[2];
    NUVEC local_position;
    u32 reserved_10c;
    nuhspecial_s special[2];
    f32 minimum_scale;
    f32 scale;
    f32 scale_speed;
    f32 secondary_scale;
    u8 active;
    u8 type;
    i16 has_second_special;
    i16 platform_id;
    u8 reserved_13e[2];
};
DECOMP_ASSERT(sizeof(AnakinCEntry) == 0x140, "Anakin C entry ABI");
extern GameObject_s *volatile AnakinC;

struct DoorSetupEntry {
    char *first_name;
    char *second_name;
    f32 start_scale;
    f32 scale_speed;
    f32 type;
    NUVEC local_position;
    f32 secondary_scale;
    f32 minimum_scale;
};
DECOMP_ASSERT(sizeof(DoorSetupEntry) == 0x28, "Door setup entry ABI");

DoorSetupEntry DoorSetupList[15] = {
    {"door1", "door1r", 15.0f, 1.0f, 0.0f, {-1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door2", "door2r", 15.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door3", "door3r", 15.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door4", "door4r", 15.0f, 1.0f, 0.0f, {-1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door5", "door5r", 15.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door6", "door6r", 15.0f, 1.0f, 0.0f, {-1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door7", "door7r", 15.0f, 1.0f, 0.0f, {1.0f, 1.0f, 0.0f}, -0.5f, 0.0f},
    {"door8", "door8r", 15.0f, 1.0f, 0.0f, {0.0f, -1.0f, 0.0f}, -0.5f, 0.0f},
    {"door9", "door9r", 15.0f, 1.0f, 0.0f, {0.0f, 1.0f, 0.0f}, -0.5f, 0.0f},
    {"door10", "door10r", 15.0f, 1.0f, 0.0f, {0.0f, -1.0f, 0.0f}, -0.5f, 0.0f},
    {"door11", "door11r", 15.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door12", "door12r", 15.0f, 1.0f, 0.0f, {-1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door15", "door15r", 15.0f, 1.0f, 0.0f, {1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {"door16", "door16r", 15.0f, 1.0f, 0.0f, {-1.0f, 0.0f, 0.0f}, -0.5f, 0.0f},
    {},
};

void ChrisAnakinCReset() {
    AnakinCEntry *entry = reinterpret_cast<AnakinCEntry *>(AnakinC);
    DoorSetupEntry *setup = DoorSetupList;
    i32 count = 0;
    while (setup->first_name != NULL) {
        if (NuSpecialFind(WORLD->current_gscn, &entry->special[0], setup->first_name, 1) != 0 &&
            NuSpecialExistsFn(&entry->special[0]) != 0) {
            if (NuSpecialFind(WORLD->current_gscn, &entry->special[1], setup->second_name, 1) != 0)
                entry->has_second_special = static_cast<i16>(NuSpecialExistsFn(&entry->special[1]));

            NuMtxSetIdentity(&entry->transform_matrix[0]);
            entry->transform_matrix[0] = *NuSpecialGetMtx(&entry->special[0]);
            entry->draw_matrix[0] = entry->transform_matrix[0];
            if (entry->has_second_special != 0) {
                NuMtxSetIdentity(&entry->transform_matrix[1]);
                entry->transform_matrix[1] = *NuSpecialGetMtx(&entry->special[1]);
                entry->draw_matrix[1] = entry->transform_matrix[1];
            }
            entry->platform_id = static_cast<i16>(FindPlatInst(NuSpecialGetInstanceix(&entry->special[0])));
            entry->active = 1;
            entry->type = static_cast<u8>(*reinterpret_cast<u16 *>(&setup->type));
            entry->local_position = setup->local_position;
            entry->scale = setup->start_scale;
            entry->scale_speed = setup->scale_speed;
            entry->minimum_scale = setup->minimum_scale;
            entry->secondary_scale = setup->secondary_scale;
            ++entry;
            ++count;
        }
        if (count > 11)
            return;
        ++setup;
    }
    while (count < 12) {
        entry->active = 0;
        ++entry;
        ++count;
    }
}

void ChrisAnakinDReset(WORLDINFO_s *world) {
    ResetSpaceLevel(world, world->space_level);
}

void ChrisAnakinBUpdate() {
}

void ChrisAnakinCUpdate() {
    AnakinCEntry *entries = reinterpret_cast<AnakinCEntry *>(AnakinC);
    AnakinCEntry *end = entries + 12;
    NUVEC scaled, position;
    for (AnakinCEntry *entry = entries; entry != end; ++entry) {
        if (entry->active == 0)
            continue;
        const f32 next_scale = entry->scale - FRAMETIME * entry->scale_speed;
        if (entry->minimum_scale < next_scale)
            entry->scale = next_scale;
        else
            entry->scale = entry->minimum_scale;
        scaled.x = entry->local_position.x * entry->scale;
        scaled.y = entry->local_position.y * entry->scale;
        scaled.z = entry->local_position.z * entry->scale;
        NuVecMtxTransform(&position, &scaled, &entry->transform_matrix[0]);
        memcpy(&entry->draw_matrix[0].m30, &position, sizeof(position));
        entry->draw_matrix[0].m33 = 1.0f;
        NuSpecialSetDrawMtx(&entry->special[0], &entry->draw_matrix[0]);
        if (entry->has_second_special != 0) {
            NuVecMtxTransform(&position, &scaled, &entry->transform_matrix[1]);
            memcpy(&entry->draw_matrix[1].m30, &position, sizeof(position));
            entry->draw_matrix[1].m33 = 1.0f;
            NuSpecialSetDrawMtx(&entry->special[1], &entry->draw_matrix[1]);
        }
    }
}

void ChrisAnakinDUpdate(WORLDINFO_s *) {
}

void ChrisAfterBurnerCam(nuvec_s *, nuvec_s *camera) {
    *camera = WORLD->space_level->camera_origin;
}

void ChrisAllocLevelStuff(WORLDINFO_s *world) {
    i32 *has_chris_data = reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(world) + 0x511c);
    *has_chris_data = 1;
    if (world->current_level == DOGFIGHTA_LDATA) {
        spacelevel_s *space = static_cast<spacelevel_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108,
                                                                          0x63ef4));
        world->space_level = space;
        space->reset_buffer = reinterpret_cast<u8 *>(space) + 0x5ce90;
        space->reset_buffer_count = 0x100;
        *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(space) + 0x62ee4) = 1.0f;
        const f32 speed = world->sock_sys->sock[0].current_speed;
        if (speed != 0.0f)
            *reinterpret_cast<f32 *>(reinterpret_cast<u8 *>(space) + 0x62ee4) = speed / 11.0f;
        *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(space) + 0x62ef0) = 0;
    } else if (world->current_level == PODRACEA_LDATA || world->current_level == PODRACEB_LDATA ||
               world->current_level == PODRACEC_LDATA) {
        world->podrace = GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 0xaf24);
    } else {
        *has_chris_data = 0;
    }
}

i32 DidBoltHitChrisJobby(WORLDINFO_s *, BOLT_s *) {
    return 0;
}

i32 ChrisExtraBoltCollision(BOLT_s *, nuvec_s *) {
    STUBBED();
    return 0;
}

void ChrisGetSpaceShipMatrix(GameObject_s *object, numtx_s *matrix) {
    *matrix = object->apiobj.field_0xb8;
    NuMtxPreRotateY(matrix, 0x8000);
}

void ChrisGetTargetedSpaceShipMatrix(GameObject_s *object, numtx_s *matrix) {
    *matrix = object->apiobj.field_0xb8;
    NuMtxPreRotateY(matrix, 0x8000);
}
