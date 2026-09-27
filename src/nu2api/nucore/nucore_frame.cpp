#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nu3d/nuocclusion.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "gamelib/nuwind/nuwind.h"
void bgSuspendMain(i32);
void NuPadRecordEndFrame();

i32 NuFrameEndBgLoadPS(i32 minimum_delay) {
    i32 delay = 0;
    NUTIME now;
    NUTIME elapsed;
    NuTimeGet(&now);
    NuTimeSub(&elapsed, &now, &nuapi.time2);
    const i32 scanlines = static_cast<i32>(NuTimeScanlines(&elapsed));
    if (nuapi.fps == 60.0f) {
        delay = 0xff - scanlines;
    } else if (nuapi.fps == 50.0f) {
        delay = 0x131 - scanlines;
    } else if (nuapi.fps == 30.0f) {
        delay = 0x1e0 - scanlines;
    } else if (nuapi.fps == 25.0f) {
        delay = 0x244 - scanlines;
    }
    if (delay >= minimum_delay) {
        bgSuspendMain(delay);
        return 0;
    }
    return 1;
}

extern "C" {
    i32 NuHasError();
    void NuMtlAnimate(f32);
    void NuTexAnimProcess(f32);
    void NuPad_Interface_Render();
    void NuPadUpdatePads();
    extern void (*preRenderFlashingHack)();
    extern void (*postRenderFlashingHack)();
    extern void (*nuapi_endframe_callbackfn)();
    static i32 min_delay = 20;
    f32 NuFrameEnd(void) {
        static i32 ShowingError = 0; // _ZZ10NuFrameEndE12ShowingError

        i32 done = 0;
        i32 has_error = NuHasError();

        if (nuapi.max_fps != 0) {
            // Wait until at least 1/max_fps seconds have elapsed since frame
            // begin (nuapi.time), then record the elapsed time.
            f32 target = 1.0f / (f32)nuapi.max_fps;

            NUTIME now;
            NUTIME delta;
            NuTimeGet(&now);
            NuTimeSub(&delta, &now, &nuapi.time);
            while (target > NuTimeSeconds(&delta)) {
                NuTimeGet(&now);
                NuTimeSub(&delta, &now, &nuapi.time);
            }

            nuapi.frametime = NuTimeSeconds(&delta);
        }

        NuMtlAnimate(nuapi.frametime);
        NuTexAnimProcess(nuapi.frametime);
        NuWindAnimate(nuapi.wind, nuapi.frametime);
        NuOcclusionManagerEndFrame();
        NuPadRecordEndFrame();
        NuTimeBarSetRender(-1);
        NuPad_Interface_Render();

        done = 1;

        if (preRenderFlashingHack != NULL) {
            preRenderFlashingHack();
        }

        NuRndrSwapScreenEx(-1, nuapi_endframe_callbackfn);

        NUTIME end;
        NUTIME delta2;
        NuTimeGet(&end);
        NuTimeSub(&delta2, &end, &nuapi.time);
        nuapi.frametime = NuTimeSeconds(&delta2);
        nuapi.time = end;

        if (nuapi.frametime > 0.1f) {
            nuapi.frametime = 0.1f;
        }

        NuTimeGet(&nuapi.time2);

        if (done) {
            bgSuspendMain(min_delay);
        }

        if (postRenderFlashingHack != NULL) {
            postRenderFlashingHack();
        }

        NuPadUpdatePads();
        nuapi.frame_count++;
        nuapi.nuframe_begin_cnt--;

        if (ShowingError == 0 && has_error != 0) {
            ShowingError = 1;
            // Original shows an error dialog here.
        } else if (ShowingError != 0) {
            ShowingError = 0;
        }

        nuapi.disable_os_menu_freeze = 0;
        return nuapi.frametime;
    }
    void NuFrameSetMinDelay(i32 delay) {
        min_delay = delay;
    }
}
