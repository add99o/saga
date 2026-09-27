#pragma once

#include "decomp.h"

struct AIPATHINFO_s;
struct AILOCATOR_s;
struct GameObject_s;
struct LEVEL_PROGRESS_s;
struct nuvec_s;

void StoreProgressAICharacter(LEVEL_PROGRESS_s *progress);
AILOCATOR_s *getSpawnLocator(f32 clip_radius, char *name);
i32 SpawnMeleeCreatureType(i32 type);
