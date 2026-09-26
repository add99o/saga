#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void Batarangs_CheckLostData(void *);
EXPLOSION *Detonate(nuvec_s *, u16);
void Detonator_Detonate(DETONATOR_s *);

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

void Detonator_MoveCode(GameObject_s *) {
    STUBBED();
}

static inline void DetonatorConsiderNearest(DETONATOR_s *detonator, NUVEC *position, GameObject_s *owner,
                                            f32 &nearest_distance, DETONATOR_s *&nearest) {
    if (detonator->active != 0 && (owner == NULL || detonator->object == owner)) {
        const f32 distance = NuVecDistSqr(position, &detonator->position, NULL);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest = detonator;
        }
    }
}

DETONATOR_s *Detonator_FindNearest(nuvec_s *position, float radius, GameObject_s *owner) {
    f32 nearest_distance = radius == 0.0f ? 1000000000.0f : radius * radius;
    DETONATOR_s *nearest = NULL;
    if (owner != NULL) {
        DetonatorConsiderNearest(&Detonator[0], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[1], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[2], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[3], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[4], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[5], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[6], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[7], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[8], position, owner, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[9], position, owner, nearest_distance, nearest);
    } else {
        DetonatorConsiderNearest(&Detonator[0], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[1], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[2], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[3], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[4], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[5], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[6], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[7], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[8], position, NULL, nearest_distance, nearest);
        DetonatorConsiderNearest(&Detonator[9], position, NULL, nearest_distance, nearest);
    }
    return nearest;
}
