#pragma once
#ifndef GHOST_SPAWN_H
#define GHOST_SPAWN_H

#include "ghost.h"

struct GhostSpawnPosition {
    int x, y;
    int releaseDelay;
};

struct SpawnManager {
    GhostSpawnPosition blinkySpawn;
    GhostSpawnPosition pinkySpawn;
    GhostSpawnPosition inkySpawn;
    GhostSpawnPosition clydeSpawn;
};

SpawnManager getSpawnPositions(int level);
void resetAllGhostsToSpawn(Ghost ghosts[4], const SpawnManager& spawnMgr, const DifficultyParams& params);
void resetGhostToSpawn(Ghost& ghost, const SpawnManager& spawnMgr, const DifficultyParams& params);

#endif