#pragma once

#include "decomp.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

// Returns 0 without a successful playback call, otherwise 1 (skipped) or 2
// (not skipped). The resulting regional movie/subtitle paths must fit 256 bytes.
i32 Movie_Play(char *name, VARIPTR *buffer, VARIPTR *buffer_end, f32 frame_time, i32 (*input_fn)(), f32 volume);

void Movies_ConfigureList(char *, VARIPTR *, VARIPTR *);
