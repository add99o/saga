#include <math.h>
#include <stdlib.h>
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/numath.h"
#include "decomp.h"
#include "globals.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion/contexts.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/gizmo/base/gizactions.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/core/terrain.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nutrig.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

pushblock_s *BlockInBlock(WORLDINFO_s *, pushblock_s *, i32, pushblock_s **);

enum PushBlockCompletionFlags {
    PUSH_BLOCK_FIRST_OUTPUT_FLAG = 1 << 3,
    PUSH_BLOCK_ANY_OUTPUT_MASK = 0x7f8,
};

u32 (*CanPushBlocksFn)(GameObject_s *) = NULL;
i32 pushposincrease;
i32 runoutofpostabspace;

void SetAnimFrame(nuhspecial_s *, f32);
void ResetPushProgress(WORLDINFO_s *, void *);
void ResetSinglePushBlockHeight(WORLDINFO_s *, pushblock_s *, i32);
void AlertSurroundingCreatures(GameObject_s *, NUVEC *);
i32 TerrainBlockOnBlock(WORLDINFO_s *, pushblock_s *, NUVEC *, f32 *);
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);

__attribute__((force_align_arg_pointer)) void KnockPushBlock(pushblock_s *block, nuvec_s *direction) {
    if (block == NULL) {
        return;
    }
    block->velocity.x = -direction->x;
    block->velocity.y = -direction->y;
    block->velocity.z = -direction->z;
    NuVecScale(&block->velocity, &block->velocity, 0.005f);
    block->runtime_flags_0c8 |= 0x80;
}

i32 NewBlockAction(GameObject_s *object) {
    i32 actions[3];
    i32 count = 0;
    for (i32 action = 0; action < apicharsys->model_id_capacity && count < 3; ++action) {
        if ((ActionInfo[action].flags & 8) != 0 && object->apiobj.character_model->model_data_b[action] != NULL) {
            actions[count++] = action;
        }
    }
    if (count == 0)
        return 0;
    i32 action = actions[0];
    if (count != 1) {
        do {
            action = actions[qrand() / (0xffff / count + 1)];
        } while (action == object->previous_block_animation);
    }
    object->previous_block_animation = action;
    object->context_animation = action;
    return 1;
}

pushblock_s *NearestPushBlock(WORLDINFO_s *world, nuvec_s *position, float range) {
    if (position == NULL || world == NULL)
        return NULL;
    const NUVEC minimum = {position->x - range, position->y - range, position->z - range};
    const NUVEC maximum = {position->x + range, position->y + range, position->z + range};
    pushblock_s *nearest = NULL;
    f32 nearest_distance = 1000000000.0f;
    for (i32 i = 0; i < world->push_block_count; ++i) {
        pushblock_s *block = &world->push_blocks[i];
        if ((block->packed_state_flags & 0x1040100) != 0x1040000)
            continue;
        NUVEC *candidate = block->position;
        if (candidate->x < minimum.x || candidate->x > maximum.x || candidate->z < minimum.z ||
            candidate->z > maximum.z || candidate->y < minimum.y || candidate->y > maximum.y)
            continue;
        const f32 distance = NuVecDistSqr(position, candidate, NULL);
        if (distance < nearest_distance) {
            nearest = block;
            nearest_distance = distance;
        }
    }
    return nearest;
}

extern "C" f32 NuFmax(f32, f32);
void NewBuzz(nupad_s *, f32, i32);
void PushSeekComplete(pushblock_s *block, i32 index) {
    block->completion_flags =
        (block->completion_flags & 0xf807) | ((((block->completion_flags >> 3) | (1u << (index & 31))) & 0xff) << 3);
    block->runtime_flags_0c9 &= ~2;
    if (!(block->flags_0cb & 0x20)) {
        block->flags_0cb |= 2;
        block->runtime_flags_0c9 |= 1;
    }
    if (block->pushing_object)
        NewBuzz(block->pushing_object->pad_gamepad->pad, 0.1f, 0);
}

i32 OtherBlockInRange(WORLDINFO_s *world, pushblock_s *block, nuvec_s *position, i32 excluded) {
    NUVEC centre;
    f32 radius;
    NuSpecialGetRadius(&block->special, &centre, &radius);
    radius *= radius;
    for (i32 i = 0; i < world->push_block_count; ++i) {
        if (i == excluded)
            continue;
        pushblock_s *other = &world->push_blocks[i];
        if ((other->packed_state_flags & 0x01040000) != 0x01040000 || (other->flags_0cb & 4))
            continue;
        if (!NuSpecialGetVisibilityFn(&other->special))
            continue;
        NUVEC minimum, maximum;
        NuSpecialGetBounds(&other->special, &minimum, &maximum);
        NuFmax(fabsf(minimum.x), maximum.x);
        NuFmax(fabsf(minimum.z), maximum.z);
        f32 other_radius;
        NuSpecialGetRadius(&other->special, &centre, &other_radius);
        centre = *other->position;
        f32 x = centre.x - position->x, z = centre.z - position->z;
        f32 distance = (x * x + z * z) - other_radius * other_radius;
        if (radius >= distance || 0.0f >= distance)
            return 1;
    }
    return 0;
}

void ResetSinglePushBlock(WORLDINFO_s *, pushblock_s *block, i32) {
    block->platform_id = FindPlatInst(NuSpecialGetInstanceix(&block->special));
    SetAnimFrame(&block->special, 1.0f);
    block->runtime_flags_0c8 &= 0xf2;
    block->flags_0cb &= ~2u;
    block->runtime_flags_0c9 &= 0xf0;
    block->completion_flags &= 0xf807;
    block->packed_state_flags &= 0xfffc7fff;
    block->pushing_object = NULL;

    if (NuSpecialExistsFn(&block->special) == 0) {
        block->bounds_min = v000;
        block->bounds_max = v000;
        block->position = &v000;
        return;
    }

    NuSpecialGetBounds(&block->special, &block->bounds_min, &block->bounds_max);
    NUMTX *matrix = NuSpecialGetInstanceMtx(&block->special);
    block->position = reinterpret_cast<NUVEC *>(&matrix->m30);
    if ((block->runtime_flags_0c9 & 0x70) != 0 && runoutofpostabspace == 0) {
        *block->position = block->snap_positions[0];
    }
}

pushblock_s *NearestFacingPushBlock(WORLDINFO_s *world, GameObject_s *object, float max_distance_squared) {
    if (world == NULL || world->push_blocks == NULL || world->push_block_count == 0 || LEGOCONTEXT_PUSH == -1 ||
        static_cast<i8>(object->field_0x7a5) != LEGOCONTEXT_PUSH)
        return NULL;

    const NUVEC &object_position = object->apiobj.collision_position;
    const f32 upper_y = object->apiobj.field_0x194;
    const f32 lower_y = object->apiobj.lower_position.y;
    const f32 half_height = (upper_y - lower_y) * 0.5f;
    const f32 minimum_y = upper_y - half_height;
    const f32 maximum_y = lower_y + half_height;
    const u16 facing = object->apiobj.movement_facing_angle;
    i32 direction = 2;
    if (facing >= 0x3c72 && facing <= 0x438d)
        direction = 0;
    else if (facing >= 0x7c72 && facing <= 0x838d)
        direction = 3;
    else if (facing >= 0xbc72 && facing <= 0xc38d)
        direction = 1;

    pushblock_s *nearest = NULL;
    f32 best_distance_squared = 1000000000.0f;
    for (i32 i = 0; i < world->push_block_count; ++i) {
        pushblock_s *block = &world->push_blocks[i];
        if (NuSpecialExistsFn(&block->special) != 0 &&
            (NuSpecialGetOnScreenFn(&block->special) == 0 || NuSpecialGetVisibilityFn(&block->special) == 0))
            continue;
        if ((block->packed_state_flags & 0x4000100) != 0 || (block->flags_0cb & 2) != 0 || block->position == NULL)
            continue;

        const NUVEC &position = *block->position;
        const f32 block_low = position.y - fabsf(block->bounds_min.y);
        const f32 block_high = position.y + fabsf(block->bounds_max.y);
        if (maximum_y < block_low || minimum_y > block_high)
            continue;

        const f32 delta_x = position.x - object_position.x;
        const f32 delta_z = position.z - object_position.z;
        const f32 half_x = fabsf(block->bounds_min.x);
        const f32 half_z = fabsf(block->bounds_min.z);
        if ((direction == 0 || direction == 3) ? fabsf(delta_z) > half_z + 0.5f
                                               : fabsf(delta_x) > half_x + 0.5f)
            continue;
        if ((direction == 0 && (block->flags_0ca & 0x50) == 0x50) ||
            (direction == 1 && (block->flags_0ca & 0x90) == 0x90) ||
            (direction == 2 && (block->flags_0ca & 0xa0) == 0xa0) ||
            (direction == 3 && (block->flags_0ca & 0x60) == 0x60))
            continue;

        NUVEC difference = {delta_x, position.y - object_position.y, delta_z};
        const i32 angle = NuAtan2D(delta_x, delta_z);
        NuVecRotateY(&difference, &difference, 0x4000 - (angle & 0xffff));
        const i16 facing_difference = static_cast<i16>(angle - facing);
        if (abs(static_cast<i32>(facing_difference)) > 0x2000)
            continue;
        const f32 distance_squared = delta_x * delta_x + delta_z * delta_z;
        if (distance_squared < max_distance_squared && distance_squared < best_distance_squared) {
            best_distance_squared = distance_squared;
            nearest = block;
        }
    }
    return nearest;
}

void GizmoPushBlockInitAndReset(WORLDINFO_s *world, void *progress) {
    world->push_block_position_count = 0;
    runoutofpostabspace = 0;
    pushposincrease = 0;
    world->push_block_positions = reinterpret_cast<NUVEC *>(world->giz_buffer.void_ptr);
    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr + world->current_level->max_push_block_end_pos * sizeof(NUVEC), 4);
    ResetPushProgress(world, progress);

    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *block = &world->push_blocks[index];
        block->runtime_flags_0c9 &= 0x8f;
        if (NuSpecialGetInstAnim(&block->special) != NULL) {
            i32 positions = static_cast<i32>(NuSpecialGetAnimEndFrame(&block->special)) & 7;
            block->output_count = positions;
            block->runtime_flags_0c9 = (block->runtime_flags_0c9 & 0x8f) | ((positions & 7) << 4);
            const i32 required = world->push_block_position_count + positions;
            if (required > world->current_level->max_push_block_end_pos) {
                pushposincrease += runoutofpostabspace == 0
                                       ? required - world->current_level->max_push_block_end_pos
                                       : positions;
                runoutofpostabspace = 1;
            } else if (positions != 0 && runoutofpostabspace == 0) {
                block->snap_positions = &world->push_block_positions[world->push_block_position_count];
                for (i32 position = 0; position < positions; ++position) {
                    NUMTX evaluated;
                    EvalAnim(&block->special, static_cast<f32>(position + 1), &evaluated, 0);
                    block->snap_positions[position] = *reinterpret_cast<NUVEC *>(&evaluated.m30);
                    block->snap_positions[position].x += NuSpecialGetMtx(&block->special)->m30;
                    block->snap_positions[position].y += NuSpecialGetMtx(&block->special)->m31;
                    block->snap_positions[position].z += NuSpecialGetMtx(&block->special)->m32;
                    ++world->push_block_position_count;
                }
            }
        }
        ResetSinglePushBlock(world, block, index);
    }

    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *block = &world->push_blocks[index];
        block->velocity = v000;
        block->target_velocity = v000;
        block->snap_origin = *block->position;

        const f32 left = fabsf(block->bounds_min.x) - 0.006f;
        const f32 right = fabsf(block->bounds_max.x) - 0.006f;
        const f32 back = fabsf(block->bounds_min.z) - 0.006f;
        const f32 front = fabsf(block->bounds_max.z) - 0.006f;
        const f32 y = block->position->y - fabsf(block->bounds_min.y) + 0.026f;
        NUVEC corners[4] = {
            {block->position->x - left, y, block->position->z - back},
            {block->position->x + right, y, block->position->z - back},
            {block->position->x + right, y, block->position->z + front},
            {block->position->x - left, y, block->position->z + front},
        };
        f32 heights[4];
        PlatOnOff(block->platform_id, 0);
        for (i32 corner = 0; corner < 4; ++corner) {
            NewTerrPlatformsOff();
            heights[corner] = GameShadow(NULL, &corners[corner], 0.1f, -1);
            block->terrain_info[corner] = static_cast<i8>(ShadowInfo());
            block->extra_terrain_info[corner] = static_cast<i8>(EShadowInfo());
        }
        PlatOnOff(block->platform_id, 1);

        if (heights[0] == heights[1] && heights[1] == heights[2] && heights[2] == heights[3]) {
            NewTerrPlatformsOff();
            block->ground_height = GameShadow(NULL, block->position, 5.0f, -1);
            if (block->ground_height == 2000000.0f) {
                block->ground_height = 0.0f;
            }
        } else {
            block->ground_height = (heights[0] + heights[1] + heights[2] + heights[3]) * 0.25f;
        }
        TerrainBlockOnBlock(world, block, corners, heights);
        block->previous_extra_terrain_info = *reinterpret_cast<i32 *>(block->extra_terrain_info);
    }

    for (i32 index = 0; index < world->push_block_count; ++index) {
        ResetSinglePushBlockHeight(world, &world->push_blocks[index], index);
    }
}

void ResetSinglePushBlockHeight(WORLDINFO_s *world, pushblock_s *block, i32 index) {
    BlockInBlock(world, block, index, &block->block_below);

    const f32 epsilon = 0.01f;
    f32 support_height;
    if (block->block_below != NULL) {
        pushblock_s *support = block->block_below;
        support_height = support->bounds_max.y + support->position->y + epsilon;
        block->support_height = support_height;
    } else {
        support_height = block->support_height;
    }

    NUVEC *position = block->position;
    f32 position_y = position->y;
    f32 penetration = block->bounds_min.y + position_y - support_height;
    block->vertical_penetration = penetration;
    if (penetration > epsilon) {
        position->y = position_y - penetration;
        block->vertical_penetration = 0.0f;
    }
}

i32 GizPushBlock_EndFrameCompleted(pushblock_s *push_block, i32 output_index) {
    if (push_block == NULL) {
        return -1;
    }

    if (output_index == 0) {
        return (push_block->completion_flags & PUSH_BLOCK_ANY_OUTPUT_MASK) != 0;
    }
    const u8 completed_outputs = push_block->completion_flags / PUSH_BLOCK_FIRST_OUTPUT_FLAG;
    return (completed_outputs >> output_index) & 1;
}

i32 PushBlock(GameObject_s *object) {
    pushblock_s *block = NearestFacingPushBlock(WORLD, object, 2.0f);
    if (block != NULL) {
        block->pushing_object = object;
        block->runtime_flags_0c8 |= 1;
        AlertSurroundingCreatures(object, &object->apiobj.collision_position);
        return 1;
    }
    object->field_0x7a5 = 0xff;
    return 0;
}
