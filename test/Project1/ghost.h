#pragma once
#ifndef GHOST_H
#define GHOST_H

#include "logic.h"

enum GhostType {
    BLINKY,  // Red   – targets Pac-Man's exact position
    PINKY,   // Pink  – targets 4 tiles ahead of Pac-Man
    INKY,    // Cyan  – flanks using Blinky+Pac dir
    CLYDE    // Orange– chases if far, retreats if close (8-tile rule)
};

enum GhostMode {
    SCATTER,
    CHASE,
    FRIGHTENED
};

struct Ghost {
    int x, y;
    int prevX, prevY;
    int scatterX, scatterY;
    GhostType type;
    GhostMode mode;
    char prevDir;
    bool active;
    int frightenedTimer;
    int releaseTimer;
};

struct DifficultyParams {
    int ghostMoveInterval;   // ms between ghost moves
    int frightenedDuration;  // ticks ghosts stay blue (0 = never)
    int scatterTicks;
    int chaseTicks;
    int pinkyRelease;
    int inkyRelease;
    int clydeRelease;
};

void initGhosts(Ghost ghosts[4], const DifficultyParams& params);
DifficultyParams getDifficultyParams(int level);

void moveGhost(Ghost& ghost, char maze[ROWS][COLS + 1],
    int pacX, int pacY, char pacDir,
    const Ghost* blinky);

void frightenGhosts(Ghost ghosts[4], int frightenedDuration);
void updateFrightenedTimers(Ghost ghosts[4]);
void updateModeCycle(Ghost ghosts[4], int tick, const DifficultyParams& params);
void updateReleaseTimers(Ghost ghosts[4]);

#endif // GHOST_H