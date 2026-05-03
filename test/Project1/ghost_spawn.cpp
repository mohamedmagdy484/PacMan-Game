#include "ghost_spawn.h"

SpawnManager getSpawnPositions(int level) {
    SpawnManager manager;
    manager.blinkySpawn = { 10, 7,  0 };
    manager.pinkySpawn = { 9, 9, 30 };
    manager.inkySpawn = { 10, 9, 60 };
    manager.clydeSpawn = { 11, 9, 90 };

    if (level > 5) {
        manager.pinkySpawn.releaseDelay = 20;
        manager.inkySpawn.releaseDelay = 40;
        manager.clydeSpawn.releaseDelay = 60;
    }
    return manager;
}

void resetAllGhostsToSpawn(Ghost ghosts[4], const SpawnManager& spawnMgr, const DifficultyParams& params) {
    ghosts[BLINKY].x = spawnMgr.blinkySpawn.x; ghosts[BLINKY].y = spawnMgr.blinkySpawn.y;
    ghosts[BLINKY].prevX = ghosts[BLINKY].x;   ghosts[BLINKY].prevY = ghosts[BLINKY].y;
    ghosts[BLINKY].mode = CHASE; ghosts[BLINKY].active = true;
    ghosts[BLINKY].releaseTimer = spawnMgr.blinkySpawn.releaseDelay;
    ghosts[BLINKY].frightenedTimer = 0;

    ghosts[PINKY].x = spawnMgr.pinkySpawn.x; ghosts[PINKY].y = spawnMgr.pinkySpawn.y;
    ghosts[PINKY].prevX = ghosts[PINKY].x;   ghosts[PINKY].prevY = ghosts[PINKY].y;
    ghosts[PINKY].mode = SCATTER; ghosts[PINKY].active = true;
    ghosts[PINKY].releaseTimer = params.pinkyRelease;
    ghosts[PINKY].frightenedTimer = 0;

    ghosts[INKY].x = spawnMgr.inkySpawn.x; ghosts[INKY].y = spawnMgr.inkySpawn.y;
    ghosts[INKY].prevX = ghosts[INKY].x;   ghosts[INKY].prevY = ghosts[INKY].y;
    ghosts[INKY].mode = SCATTER; ghosts[INKY].active = true;
    ghosts[INKY].releaseTimer = params.inkyRelease;
    ghosts[INKY].frightenedTimer = 0;

    ghosts[CLYDE].x = spawnMgr.clydeSpawn.x; ghosts[CLYDE].y = spawnMgr.clydeSpawn.y;
    ghosts[CLYDE].prevX = ghosts[CLYDE].x;   ghosts[CLYDE].prevY = ghosts[CLYDE].y;
    ghosts[CLYDE].mode = SCATTER; ghosts[CLYDE].active = true;
    ghosts[CLYDE].releaseTimer = params.clydeRelease;
    ghosts[CLYDE].frightenedTimer = 0;
}

void resetGhostToSpawn(Ghost& ghost, const SpawnManager& spawnMgr, const DifficultyParams& params) {
    switch (ghost.type) {
    case BLINKY: ghost.x = spawnMgr.blinkySpawn.x; ghost.y = spawnMgr.blinkySpawn.y; ghost.releaseTimer = spawnMgr.blinkySpawn.releaseDelay; break;
    case PINKY:  ghost.x = spawnMgr.pinkySpawn.x;  ghost.y = spawnMgr.pinkySpawn.y;  ghost.releaseTimer = params.pinkyRelease; break;
    case INKY:   ghost.x = spawnMgr.inkySpawn.x;   ghost.y = spawnMgr.inkySpawn.y;   ghost.releaseTimer = params.inkyRelease;  break;
    case CLYDE:  ghost.x = spawnMgr.clydeSpawn.x;  ghost.y = spawnMgr.clydeSpawn.y;  ghost.releaseTimer = params.clydeRelease; break;
    }
    ghost.prevX = ghost.x; ghost.prevY = ghost.y;
    ghost.mode = SCATTER; ghost.active = true; ghost.frightenedTimer = 0;
}