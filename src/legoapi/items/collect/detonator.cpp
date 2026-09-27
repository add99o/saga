#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/ai/game/gameantinode.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/action_info.h"
#include "legoapi/characters/motion/animlist.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void Batarangs_CheckLostData(void *);
EXPLOSION *Detonate(nuvec_s *, u16);
void Detonator_Detonate(DETONATOR_s *);
DETONATOR_s *Detonator_FindNearest(nuvec_s *, float, GameObject_s *);
void FastWeaponIn(GameObject_s *, i32);
void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);

struct DetonatorHitData {
    u8 field_0x00[0xc];
    f32 field_0x0c;
    u8 field_0x10[0x10];
    f32 field_0x20;
};

void Detonators_Draw() {
    if (!WORLD->lev_objs[0xec].active) {
        return;
    }

    for (i32 i = 0; i < 10; ++i) {
        DETONATOR_s &detonator = Detonator[i];
        detonator.draw_result = 0;
        if (!detonator.active) {
            continue;
        }

        NUMTX_ALIGNED16 matrix;
        NuMtxSetRotationY(&matrix, detonator.rotation_y);
        NuMtxRotateZ(&matrix, detonator.rotation_z);
        NuMtxRotateX(&matrix, detonator.rotation_x);
        NuMtxTranslate(&matrix, &detonator.field_0x0c);
        NuSpecialDrawAt(&WORLD->lev_objs[0xec].special, &matrix);

        const bool flicker_on = PickUpFlickerTest <= PickupFlickerFrame % PickUpFlickerFrames;
        const bool attached =
            detonator.timer < 0.5f && (detonator.object == NULL || detonator.object->apiobj.field_0x287 != 0 ||
                                       detonator.object->field_0xde0 < 0.3f);
        const i32 special_index = flicker_on || attached ? 0xee : 0xef;
        if (WORLD->lev_objs[special_index].active) {
            detonator.draw_result = NuSpecialDrawAt(&WORLD->lev_objs[special_index].special, &matrix);
        }
    }
}

void Detonators_Reset() {
    memset(Detonator, 0, sizeof(Detonator));
}

void Detonators_Update() {
    for (i32 i = 0; i < 10; ++i) {
        DETONATOR_s *detonator = &Detonator[i];
        if (detonator->active == 0) {
            continue;
        }
        detonator->timer += FRAMETIME;
        NuCameraTransformScreenClip(&detonator->field_0x18, &detonator->field_0x0c, 1, NULL);
        if (detonator->timer >= 10.7f) {
            Detonator_Detonate(detonator);
        } else if (detonator->field_0x34 != NULL) {
            DetonatorHitData *hit_data = static_cast<DetonatorHitData *>(detonator->field_0x34);
            if (detonator->timer >= 10.0f || (detonator->object != NULL && detonator->object->apiobj.field_0x287 == 0 &&
                                              detonator->object->field_0xde0 >= 0.3f)) {
                hit_data->field_0x0c = 0.75f;
            } else {
                hit_data->field_0x0c = 0.08f;
            }
        }
    }
}

void Detonator_Detonate(DETONATOR_s *detonator) {
    Batarangs_CheckLostData(detonator);
    Detonate(reinterpret_cast<nuvec_s *>(&detonator->field_0x0c), 0);
    DetonatorHitData *hit_data = static_cast<DetonatorHitData *>(detonator->field_0x34);
    detonator->active = 0;
    if (hit_data != NULL) {
        hit_data->field_0x20 = 2.0f;
        detonator->field_0x34 = NULL;
    }
}

void Detonator_MoveCode(GameObject_s *object) {
    if (object->character_context == 0x48 || object->character_context == 0x49) {
        object->field_0xe21 |= 0x80;
        object->field_0xde0 = 0.0f;
        f32 *frame = NULL;
        if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL) {
            frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
            if (frame == NULL)
                return;
        }
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer <= 0.0f) {
            object->character_context = -1;
            if (object->field_0x7a3 != 0)
                return;
        } else {
            if (object->field_0x7a3 != 0)
                return;
            if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL) {
                f32 trigger_frame = AnimListFrame(object->apiobj.character_model, object->context_animation,
                                                  object->character_context == 0x49 ? 0 : 2);
                if (frame == NULL || *frame <= 0.0f || *frame < trigger_frame)
                    return;
            } else if (object->context_animation_timer >= 0.5f)
                return;
        }

        object->field_0x7a3 = 1;
        if (object->character_context == 0x49) {
            DETONATOR_s *detonator = static_cast<DETONATOR_s *>(object->field_0x788);
            if (detonator != NULL) {
                detonator->active = 0;
                if (detonator->field_0x34 != NULL) {
                    GameAntinode_UnregisterAntiNode(
                        WORLD->game_antinode_sys, static_cast<GAMEANTINODE_s *>(detonator->field_0x34));
                    detonator->field_0x34 = NULL;
                }
            }
            return;
        }

        i32 owned = 0;
        i32 slot = -1;
#define CHECK_SLOT(index)                                                                                              \
    if (Detonator[index].active && Detonator[index].object == object) {                                                \
        owned++;                                                                                                        \
        if (owned >= 3) {                                                                                               \
            GameAudio_PlaySfx(0x32, NULL, 0, 0);                                                                        \
            return;                                                                                                     \
        }                                                                                                               \
    } else if (slot < 0)                                                                                                \
        slot = index
        CHECK_SLOT(0);
        CHECK_SLOT(1);
        CHECK_SLOT(2);
        CHECK_SLOT(3);
        CHECK_SLOT(4);
        CHECK_SLOT(5);
        CHECK_SLOT(6);
        CHECK_SLOT(7);
        CHECK_SLOT(8);
        CHECK_SLOT(9);
#undef CHECK_SLOT
        if (slot < 0) {
            GameAudio_PlaySfx(0x32, NULL, 0, 0);
            return;
        }

        DETONATOR_s *detonator = &Detonator[slot];
        detonator->field_0x00 = object->context_destination;
        detonator->active = 1;
        detonator->object = object;
        detonator->rotation_x = object->context_x_rotation;
        detonator->rotation_y = object->takeover_start_angle + object->apiobj.field_0x276;
        detonator->rotation_z = object->context_z_rotation;
        detonator->field_0x0c = {0.0f, 0.0775f, 0.0f};
        NuVecRotateZ(&detonator->field_0x0c, &detonator->field_0x0c, detonator->rotation_z);
        NuVecRotateX(&detonator->field_0x0c, &detonator->field_0x0c, detonator->rotation_x);
        NuVecAdd(&detonator->field_0x0c, &detonator->field_0x0c, &detonator->field_0x00);
        detonator->timer = 0.0f;
        detonator->field_0x34 = GameAntinode_RegisterAntiNode(WORLD->game_antinode_sys, &detonator->field_0x0c, 1.0f,
                                                               1.0f, 1.0f, 0, 0, 0.0f);
        PlaySfx(const_cast<char *>("imp_thermalDet_attach"), &detonator->field_0x00);
        return;
    }

    if (WORLD->lev_objs[0xec].active == 0 || static_cast<i8>(object->apiobj.flags_low) >= 0 ||
        (object->apiobj.field_0x27d == 0 && object->field_0xe31 != 1) ||
        (!ObjLandReady(object) && (CInfo[object->character_context].flags & 0x2000) == 0)) {
        object->field_0xde0 = 0.0f;
        object->field_0xe21 |= 0x80;
        return;
    }

    if ((object->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0) {
        if (object->field_0xde0 <= 0.0f && (object->field_0xe21 & 0x80) == 0)
            return;
        object->field_0xe21 |= 0x80;
        object->field_0xde0 += FRAMETIME;
        if (object->field_0xde0 < 1.0f)
            return;

        DETONATOR_s *oldest = NULL;
        f32 longest = -1.0f;
#define CHECK_OLDEST(index)                                                                                            \
    if (Detonator[index].active && Detonator[index].timer > longest) {                                                  \
        longest = Detonator[index].timer;                                                                                \
        oldest = &Detonator[index];                                                                                      \
    }
        CHECK_OLDEST(0);
        CHECK_OLDEST(1);
        CHECK_OLDEST(2);
        CHECK_OLDEST(3);
        CHECK_OLDEST(4);
        CHECK_OLDEST(5);
        CHECK_OLDEST(6);
        CHECK_OLDEST(7);
        CHECK_OLDEST(8);
        CHECK_OLDEST(9);
#undef CHECK_OLDEST
        if (oldest != NULL) {
            Detonator_Detonate(oldest);
            object->field_0xde0 = 0.7f;
        } else
            object->field_0xde0 = 0.0f;
        return;
    }

    if (object->field_0xde0 <= 0.0f || object->field_0xde0 >= 0.3f) {
        object->field_0xde0 = 0.0f;
        object->field_0xe21 &= ~0x80;
        return;
    }

    f32 radius = 0.0775f + object->apiobj.field_0x1dc;
    DETONATOR_s *nearby = Detonator_FindNearest(&object->apiobj.lower_position, radius, object);
    if (nearby != NULL) {
        object->field_0x7a3 = 0;
        object->character_context = 0x49;
        FastWeaponIn(object, 0);
        object->field_0x788 = nearby;
        object->context_animation = 0x66;
        u16 angle = NuAtan2D(nearby->field_0x00.x - object->apiobj.position.x,
                             nearby->field_0x00.z - object->apiobj.position.z);
        object->apiobj.movement_facing_angle = angle;
        object->context_destination.x = nearby->field_0x00.x - NuTrigTable[(angle >> 1) & 0x7fff] * 0.1684f;
        object->context_destination.z =
            nearby->field_0x00.z - NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * 0.1684f;
        object->takeover_start_angle = RotDiff(angle, nearby->rotation_y);
    } else {
        if (object->apiobj.field_0x218 == 2000000.0f ||
            object->apiobj.lower_position.y - object->apiobj.field_0x218 >= 0.155f) {
            GameAudio_PlaySfx(0x32, NULL, 0, 0);
            object->field_0xde0 = 0.0f;
            object->field_0xe21 &= ~0x80;
            return;
        }
        object->context_destination = object->apiobj.lower_position;
        object->context_destination.y = object->apiobj.field_0x218;
        if (Detonator_FindNearest(&object->context_destination, 0.155f, NULL) != NULL) {
            GameAudio_PlaySfx(0x32, NULL, 0, 0);
            object->field_0xde0 = 0.0f;
            object->field_0xe21 &= ~0x80;
            return;
        }
        object->field_0x7a3 = 0;
        object->character_context = 0x48;
        FastWeaponIn(object, 0);
        object->context_animation = 0x8d;
        object->context_x_rotation = object->field_0x1062;
        object->takeover_start_angle = qrand();
        object->context_z_rotation = object->field_0x1064;
    }

    object->apiobj.velocity.x = 0.0f;
    object->apiobj.velocity.z = 0.0f;
    if (AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 1) != NULL)
        ResetAnimPacket(&object->apiobj.anim_packet, -1);
    if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL)
        object->context_animation_timer = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
    else
        object->context_animation_timer = 1.0f;
    object->field_0xde0 = 0.0f;
    object->field_0xe21 &= ~0x80;
}

DETONATOR_s *Detonator_FindNearest(nuvec_s *position, float radius, GameObject_s *owner) {
    f32 nearest_distance = radius == 0.0f ? 1000000000.0f : radius * radius;
    DETONATOR_s *nearest = NULL;

    if (owner != NULL) {
#define CHECK_DETONATOR(index)                                                                                         \
    if (Detonator[index].active && Detonator[index].object == owner) {                                                 \
        f32 distance = NuVecDistSqr(position, &Detonator[index].field_0x00, NULL);                                      \
        if (distance < nearest_distance) {                                                                              \
            nearest_distance = distance;                                                                                \
            nearest = &Detonator[index];                                                                                 \
        }                                                                                                               \
    }
        CHECK_DETONATOR(0);
        CHECK_DETONATOR(1);
        CHECK_DETONATOR(2);
        CHECK_DETONATOR(3);
        CHECK_DETONATOR(4);
        CHECK_DETONATOR(5);
        CHECK_DETONATOR(6);
        CHECK_DETONATOR(7);
        CHECK_DETONATOR(8);
        CHECK_DETONATOR(9);
#undef CHECK_DETONATOR
    } else {
#define CHECK_DETONATOR(index)                                                                                         \
    if (Detonator[index].active) {                                                                                      \
        f32 distance = NuVecDistSqr(position, &Detonator[index].field_0x00, NULL);                                      \
        if (distance < nearest_distance) {                                                                              \
            nearest_distance = distance;                                                                                \
            nearest = &Detonator[index];                                                                                 \
        }                                                                                                               \
    }
        CHECK_DETONATOR(0);
        CHECK_DETONATOR(1);
        CHECK_DETONATOR(2);
        CHECK_DETONATOR(3);
        CHECK_DETONATOR(4);
        CHECK_DETONATOR(5);
        CHECK_DETONATOR(6);
        CHECK_DETONATOR(7);
        CHECK_DETONATOR(8);
        CHECK_DETONATOR(9);
#undef CHECK_DETONATOR
    }
    return nearest;
}
