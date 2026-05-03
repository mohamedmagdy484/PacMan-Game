#include "ghost.h"
#include <cstdlib>

// ──────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────
static int distSq(int x1, int y1, int x2, int y2) {
    return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

static char reverseDir(char dir) {
    if (dir == 'w') return 's';
    if (dir == 's') return 'w';
    if (dir == 'a') return 'd';
    if (dir == 'd') return 'a';
    return ' ';
}

static void applyDir(char dir, int x, int y, int& nx, int& ny) {
    nx = x; ny = y;
    if (dir == 'w') ny--;
    else if (dir == 's') ny++;
    else if (dir == 'a') nx--;
    else if (dir == 'd') nx++;
}

static void applyTeleport(int& x, int& y) {
    if (y == 9) {
        if (x >= COLS) x = 0;
        else if (x < 0) x = COLS - 1;
    }
}

static bool isInsideHouse(int x, int y) {
    return (y >= 8 && y <= 10 && x >= 8 && x <= 12);
}

static bool isWalkable(char maze[ROWS][COLS + 1], int x, int y, bool insideHouse) {
    if (y == 9 && (x < 0 || x >= COLS)) return true;
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return false;
    char tile = maze[y][x];
    if (tile == '#') return false;
    if (tile == '=' && !insideHouse) return false;
    return true;
}

// ──────────────────────────────────────────────────────
// Init
// ──────────────────────────────────────────────────────
void initGhosts(Ghost ghosts[4], const DifficultyParams& params) {
    ghosts[BLINKY] = { 10, 7, 10, 7, 18,  1, BLINKY, CHASE,     'd', true, 0, 0 };
    ghosts[PINKY] = { 9, 9,  9, 9,  2,  1, PINKY,  SCATTER,   'w', true, 0, params.pinkyRelease };
    ghosts[INKY] = { 10, 9, 10, 9, 18, 19, INKY,   SCATTER,   's', true, 0, params.inkyRelease };
    ghosts[CLYDE] = { 11, 9, 11, 9,  2, 19, CLYDE,  SCATTER,   's', true, 0, params.clydeRelease };
}

// ──────────────────────────────────────────────────────
// Difficulty
// ──────────────────────────────────────────────────────
DifficultyParams getDifficultyParams(int level) {
    DifficultyParams p;
    if (level == 1) {
        p = { 200, 40, 10, 200, 30, 60, 90 };
    }
    else if (level <= 4) {
        p = { 160, 30,  7, 300, 20, 40, 60 };
    }
    else if (level <= 8) {
        p = { 130, 15,  5, 400, 10, 20, 30 };
    }
    else if (level <= 12) {
        p = { 110,  5,  3, 500,  5, 10, 15 };
    }
    else {
        p = { 90,  0,  1, 999,  2,  4,  6 };
    }
    return p;
}

// ──────────────────────────────────────────────────────
// Chase targets
// ──────────────────────────────────────────────────────
static void getChaseTarget(const Ghost& ghost, int pacX, int pacY, char pacDir,
    const Ghost* blinky, int& tx, int& ty) {
    switch (ghost.type) {
    case BLINKY:
        tx = pacX; ty = pacY;
        break;
    case PINKY: {
        tx = pacX; ty = pacY;
        for (int i = 0; i < 4; i++) { int nx, ny; applyDir(pacDir, tx, ty, nx, ny); tx = nx; ty = ny; }
        break;
    }
    case INKY: {
        int pivX = pacX, pivY = pacY;
        for (int i = 0; i < 2; i++) { int nx, ny; applyDir(pacDir, pivX, pivY, nx, ny); pivX = nx; pivY = ny; }
        if (blinky) { tx = pivX + (pivX - blinky->x); ty = pivY + (pivY - blinky->y); }
        else { tx = pacX; ty = pacY; }
        break;
    }
    case CLYDE:
        if (distSq(ghost.x, ghost.y, pacX, pacY) > 64) { tx = pacX; ty = pacY; }
        else { tx = ghost.scatterX; ty = ghost.scatterY; }
        break;
    }
    if (tx < 0) tx = 0; if (tx >= COLS) tx = COLS - 1;
    if (ty < 0) ty = 0; if (ty >= ROWS) ty = ROWS - 1;
}

// ──────────────────────────────────────────────────────
// Move one ghost
// ──────────────────────────────────────────────────────
void moveGhost(Ghost& ghost, char maze[ROWS][COLS + 1],
    int pacX, int pacY, char pacDir, const Ghost* blinky) {
    ghost.prevX = ghost.x;
    ghost.prevY = ghost.y;
    if (!ghost.active) return;

    bool inside = isInsideHouse(ghost.x, ghost.y);

    if (inside) {
        if (ghost.releaseTimer > 0) return;
        if (ghost.x < 10) { ghost.x++; ghost.prevDir = 'd'; }
        else if (ghost.x > 10) { ghost.x--; ghost.prevDir = 'a'; }
        else { ghost.y--; ghost.prevDir = 'w'; }
        return;
    }

    int targetX, targetY;
    if (ghost.mode == SCATTER) { targetX = ghost.scatterX; targetY = ghost.scatterY; }
    else if (ghost.mode == CHASE) { getChaseTarget(ghost, pacX, pacY, pacDir, blinky, targetX, targetY); }
    else { targetX = -1; targetY = -1; }

    const char dirs[] = { 'w','s','a','d' };
    char forbidden = reverseDir(ghost.prevDir);
    char bestDir = ' ';
    int  bestDist = -1;
    int  validCount = 0;

    for (int i = 0; i < 4; i++) {
        char d = dirs[i];
        if (d == forbidden) continue;
        int nx, ny;
        applyDir(d, ghost.x, ghost.y, nx, ny);
        applyTeleport(nx, ny);
        if (!isWalkable(maze, nx, ny, inside)) continue;

        if (ghost.mode == FRIGHTENED) {
            validCount++;
            if (rand() % validCount == 0) bestDir = d;
        }
        else {
            int d2 = distSq(nx, ny, targetX, targetY);
            if (bestDir == ' ' || d2 < bestDist) { bestDist = d2; bestDir = d; }
        }
    }

    if (bestDir == ' ') {
        char rev = reverseDir(ghost.prevDir);
        int nx, ny;
        applyDir(rev, ghost.x, ghost.y, nx, ny);
        applyTeleport(nx, ny);
        if (isWalkable(maze, nx, ny, inside)) bestDir = rev;
    }

    if (bestDir != ' ') {
        int nx, ny;
        applyDir(bestDir, ghost.x, ghost.y, nx, ny);
        applyTeleport(nx, ny);
        ghost.x = nx; ghost.y = ny;
        ghost.prevDir = bestDir;
    }
}

// ──────────────────────────────────────────────────────
// Mode management
// ──────────────────────────────────────────────────────
void frightenGhosts(Ghost ghosts[4], int frightenedDuration) {
    if (frightenedDuration == 0) return;
    for (int i = 0; i < 4; i++) {
        if (!ghosts[i].active) continue;
        ghosts[i].mode = FRIGHTENED;
        ghosts[i].frightenedTimer = frightenedDuration;
        ghosts[i].prevDir = reverseDir(ghosts[i].prevDir);
    }
}

void updateFrightenedTimers(Ghost ghosts[4]) {
    for (int i = 0; i < 4; i++) {
        if (ghosts[i].mode != FRIGHTENED) continue;
        ghosts[i].frightenedTimer--;
        if (ghosts[i].frightenedTimer <= 0) {
            ghosts[i].mode = CHASE;
            ghosts[i].frightenedTimer = 0;
        }
    }
}

void updateReleaseTimers(Ghost ghosts[4]) {
    for (int i = 0; i < 4; i++)
        if (ghosts[i].releaseTimer > 0)
            ghosts[i].releaseTimer--;
}

void updateModeCycle(Ghost ghosts[4], int tick, const DifficultyParams& params) {
    int s = params.scatterTicks;
    int c = params.chaseTicks;
    int cycleLen = 2 * s + c;
    int phase = tick % cycleLen;

    GhostMode newMode;
    if (phase < s)     newMode = SCATTER;
    else if (phase < s + c)   newMode = CHASE;
    else                    newMode = SCATTER;

    for (int i = 0; i < 4; i++) {
        if (ghosts[i].mode == FRIGHTENED) continue;
        if (ghosts[i].mode != newMode) {
            ghosts[i].mode = newMode;
            ghosts[i].prevDir = reverseDir(ghosts[i].prevDir);
        }
    }
}