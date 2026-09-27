// Pending-transcription stand-ins (original exports without decompiled
// bodies yet). Collected here so each is an explicit, greppable TODO;
// they previously lived as anonymous extern-C stubs that shadowed real
// transcriptions elsewhere.

#include "decomp.h"
#include "globals.h"
#include "nu2api/nu3d/nutexanm.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/ShaderManagerOpenGL.h"
#include "nu2api/nu3d/nushader_plain.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuqfnt.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/nucore/nutime.h"
#include <string.h>

extern "C" void NuShaderManagerForceShader(void) {
}

extern "C" void *NuShaderManagerGetInstance(void) {
    return g_shaderManager;
}

extern "C" f32 NuShaderManagerGetShininessFactor(void) {
    return ShaderManagerTemplate<NuShaderObject>::shininessFactor;
}

extern "C" void NuShaderManagerLoadCompiledShaders(void) {
}

extern "C" void NuShaderManagerSetShininessFactor(f32 shininess) {
    ShaderManagerTemplate<NuShaderObject>::shininessFactor = shininess;
}

extern NUQFNT *system_qfont;
extern "C" i32 do_InstTree;
extern "C" i32 NuRndrBeginScene(i32 flags);
extern "C" f32 NuFrameEnd(void);
extern "C" char *NuGetErrN(i32 entry);
extern "C" NuVisibilityResult *myvisi;

namespace {
    struct NuInstanceTreeBounds {
        NUVEC center;
        f32 reserved_0c;
        NUVEC extent;
        f32 reserved_1c;
    };

    struct NuInstanceTreeNode {
        u16 bounds_index;
        u16 subtree_instance_count;
        u16 child_count;
        u16 first_child;
        u16 direct_instance_count;
        u16 first_instance;
    };

    struct NuInstanceTree {
        u32 reserved_00[2];
        u16 *instance_indices;
        NuInstanceTreeBounds *instance_bounds;
        u32 reserved_10;
        NuInstanceTreeNode *nodes;
        NuInstanceTreeBounds *node_bounds;
        u16 root_count;
        u16 reserved_1e;
        u16 *root_nodes;
        f32 *root_far_clips;
    };

    DECOMP_ASSERT(sizeof(NuInstanceTreeBounds) == 0x20, "instance tree bounds size");
    DECOMP_ASSERT(sizeof(NuInstanceTreeNode) == 0x0c, "instance tree node size");

    void MarkVisibleInstances(const NuInstanceTree *tree, u16 first, u16 count, i32 clip_result) {
        u8 *bits = static_cast<u8 *>(myvisi->visibility_context);
        for (u32 index = first; index < static_cast<u32>(first) + count; ++index) {
            const u16 instance = tree->instance_indices[index];
            bits[instance >> 2] |= static_cast<u8>(clip_result << ((instance & 3) * 2));
        }
    }

    void ClipInstTree(const NuInstanceTree *tree, i32 root) {
        f32 far_clip = tree->root_far_clips[root];
        if (far_clip == 0.0f) {
            far_clip = global_camera.far_clip;
        }

        const NuInstanceTreeNode *pending[128];
        i32 pending_count = 0;
        pending[pending_count++] = &tree->nodes[tree->root_nodes[root]];
        while (pending_count != 0) {
            const NuInstanceTreeNode *node = pending[--pending_count];
            NuInstanceTreeBounds *bounds = &tree->node_bounds[node->bounds_index];
            const i32 result = NuCameraIntersectsAABB(&bounds->center, &bounds->extent, far_clip, 0);
            if (result == 1) {
                MarkVisibleInstances(tree, node->first_instance, node->subtree_instance_count, 1);
                continue;
            }
            if (result == 0) {
                continue;
            }

            for (u32 child = 0; child < node->child_count && pending_count < 128; ++child) {
                pending[pending_count++] = &tree->nodes[node->first_child + child];
            }
            for (u32 index = 0; index < node->direct_instance_count; ++index) {
                const u32 instance_index = node->first_instance + index;
                bounds = &tree->instance_bounds[instance_index];
                const i32 instance_result = NuCameraIntersectsAABB(&bounds->center, &bounds->extent, far_clip, 1);
                MarkVisibleInstances(tree, static_cast<u16>(instance_index), 1, instance_result);
            }
        }
    }
} // namespace

extern "C" {
    NuVisibilityResult *myvisi;
    void NuVisiInstTree(void *context, NUGSCN *) {
        NuVisibilityResult *visibility = static_cast<NuVisibilityResult *>(context);
        const NuInstanceTree *tree = static_cast<const NuInstanceTree *>(visibility->instance_tree);
        const bool enabled = tree != NULL && do_InstTree != 0;
        visibility->state = static_cast<u8>((visibility->state & ~0x10) | (enabled ? 0x10 : 0));
        if (!enabled) {
            return;
        }
        myvisi = visibility;

        if (TreeInitialised != 0) {
            BuildWorldSpaceClipPlanes();
        } else {
            BuildCamSpaceClipPlanes();
        }
        for (i32 root = tree->root_count - 1; root >= 0; --root) {
            ClipInstTree(tree, root);
        }
    }
    __attribute__((optimize("O0,no-omit-frame-pointer"))) void NuErrorSleep(void) {
        f32 elapsed = 0.0f;
        i32 running = 1;
        NUPAD pad;
        memset(&pad, 0, sizeof(pad));
        NUTIME start;
        NuTimeGet(&start);

        while (elapsed < 5000.0f && running != 0) {
            f32 y = 500.0f;
            NUTIME now;
            NUTIME delta;
            NuTimeGet(&now);
            NuTimeSub(&delta, &now, &start);
            elapsed = NuTimeMilliSeconds(&delta);

            NuFrameBegin();
            NuRndrBeginScene(0);
            NuRndrClear(0xb00, 0xff00ffff, 1.0f);
            NuQFntSet(system_qfont);
            NuQFntPushPrintMode(2);
            NuQFntPushCoordinateSystem(NUQFNT_CSMODE_PS2);
            NuQFntSet(system_qfont);
            i32 i = 0;
            char *error;
            goto check_error;
        draw_error:
            NuQFntSetColour(system_qfont, 0xff000000);
            NuQFntMove(system_qfont, 0.0f, y, 0.0f);
            NuQFntPrintU(system_qfont, error);
            y += 150.0f;
        check_error:
            error = NuGetErrN(i++);
            if (*error != '\0')
                goto draw_error;
            NuQFntPopCoordinateSystem();
            NuQFntPopPrintMode();
            NuRndrEndScene();
            NuPadRead(&pad);
            if ((pad.digital_buttons & 0x40) != 0 && elapsed > 1000.0f) {
                running = 0;
            }
            NuFrameEnd();
        }
    }

    // ---------------------------------------------------------------------------
    // Thread / misc OS
    // ---------------------------------------------------------------------------
}
