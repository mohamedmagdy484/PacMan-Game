/*
 * Pac-Man – SDL2 port
 * Identical game logic to the original console version.
 * Rendering: pixel-art SDL2 (no Windows console API).
 *
 * Controls:  W/A/S/D or Arrow Keys = move
 *            P                     = pause
 *            ESC                   = quit / return to menu
 */

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>

#include "logic.h"
#include "ghost.h"
#include "ghost_spawn.h"
#include "renderer.h"

 // ──────────────────────────────────────────────────────────────────
 // Constants
 // ──────────────────────────────────────────────────────────────────
static const int PAC_MOVE_INTERVAL = 140; // ms

// ──────────────────────────────────────────────────────────────────
// Game state (all global so RestartGame() can reset cleanly)
// ──────────────────────────────────────────────────────────────────
static char            maze[ROWS][COLS + 1];
static int             pacX = 9, pacY = 15;
static int             prevPacX = 9, prevPacY = 15;
static char            currentDir = 'd';
static char            queuedDir = 'd';   // buffered input

static int             score = 0;
static int             highScore = 0;
static int             lives = 3;
static int             level = 1;

static Ghost           ghosts[4];
static DifficultyParams difficulty;
static SpawnManager    spawnManager;

// ──────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────
static bool dotsRemain() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (maze[r][c] == '.' || maze[r][c] == 'o')
                return true;
    return false;
}

static void respawnPacman() {
    pacX = 9; pacY = 15;
    currentDir = 'd'; queuedDir = 'd';
    resetAllGhostsToSpawn(ghosts, spawnManager, difficulty);
}

static void restartGame() {
    score = 0; lives = 3; level = 1;
    initializeMazeWithGhostHouse(maze);
    difficulty = getDifficultyParams(level);
    spawnManager = getSpawnPositions(level);
    initGhosts(ghosts, difficulty);
    respawnPacman();
}

// ──────────────────────────────────────────────────────────────────
// Death animation (pure SDL2 – flash Pac-Man position)
// ──────────────────────────────────────────────────────────────────
static void playDeathAnimation(SDL_Renderer* sdlRen, Renderer& ren) {
    // Flash the pac position yellow/black 6 times, then clear
    for (int i = 0; i < 7; i++) {
        Color col = (i % 2 == 0) ? Color{ 255,255,0,255 } : Color{ 0,0,0,255 };
        // Redraw full frame then overdraw pac tile
        ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
            score, highScore, lives, level,
            SDL_GetTicks(), false, false, false);
        // Overdraw pac position with flash colour
        SDL_SetRenderDrawColor(sdlRen, col.r, col.g, col.b, 255);
        SDL_Rect r = { pacX * TILE + 2, pacY * TILE + 2, TILE - 4, TILE - 4 };
        SDL_RenderFillRect(sdlRen, &r);
        SDL_RenderPresent(sdlRen);
        SDL_Delay(150);
    }
}

// ──────────────────────────────────────────────────────────────────
// Ghost eaten animation (flash white/black at ghost position)
// ──────────────────────────────────────────────────────────────────
static void playEatenAnimation(SDL_Renderer* sdlRen, Renderer& ren,
    int gx, int gy) {
    for (int i = 0; i < 6; i++) {
        ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
            score, highScore, lives, level,
            SDL_GetTicks(), false, false, false);
        Color col = (i % 2 == 0) ? Color{ 255,255,255,255 } : Color{ 0,0,0,255 };
        SDL_SetRenderDrawColor(sdlRen, col.r, col.g, col.b, 255);
        SDL_Rect r = { gx * TILE + 2, gy * TILE + 2, TILE - 4, TILE - 4 };
        SDL_RenderFillRect(sdlRen, &r);
        SDL_RenderPresent(sdlRen);
        SDL_Delay(120);
    }
}

// ──────────────────────────────────────────────────────────────────
// Ghost-return-to-spawn animation (smooth slide of eyes)
// ──────────────────────────────────────────────────────────────────
static void animateGhostReturn(SDL_Renderer* sdlRen, Renderer& ren,
    Ghost& ghost,
    int startX, int startY,
    int endX, int endY) {

    const int STEPS = 12;
    for (int i = 0; i <= STEPS; i++) {
        ghost.x = startX + (endX - startX) * i / STEPS;
        ghost.y = startY + (endY - startY) * i / STEPS;
        ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
            score, highScore, lives, level,
            SDL_GetTicks(), false, false, false);
        SDL_RenderPresent(sdlRen);
        // ── Screenshot at midpoint of return journey ─────────
        if (i == 6) {
            SDL_Surface* shot = SDL_CreateRGBSurface(0, WIN_W, WIN_H, 32,
                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
            if (shot) {
                SDL_RenderReadPixels(sdlRen, NULL, shot->format->format,
                    shot->pixels, shot->pitch);
                SDL_SaveBMP(shot, "ghost_return.bmp");
                SDL_FreeSurface(shot);
            }
        }
        // ─────────────────────────────────────────────────────

        SDL_Delay(18);
    }
    ghost.x = endX; ghost.y = endY;
}

// ──────────────────────────────────────────────────────────────────
// Collision handling 
// ──────────────────────────────────────────────────────────────────
static int checkGhostCollision() {
    for (int i = 0; i < 4; i++) {
        if (!ghosts[i].active) continue;
        if (ghosts[i].x == pacX && ghosts[i].y == pacY)         return i;
        if (ghosts[i].x == prevPacX && ghosts[i].y == prevPacY
            && ghosts[i].prevX == pacX && ghosts[i].prevY == pacY) return i;
    }
    return -1;
}

// Returns true if Pac-Man lost a life
static bool handleCollision(int idx, SDL_Renderer* sdlRen, Renderer& ren) {
    if (idx == -1) return false;

    if (ghosts[idx].mode == FRIGHTENED) {
        score += 200;

        int eX = ghosts[idx].x, eY = ghosts[idx].y;
        playEatenAnimation(sdlRen, ren, eX, eY);

        // Spawn position per ghost type
        int newX = 10, newY = 7;
        switch (ghosts[idx].type) {
        case BLINKY: newX = 10; newY = 7; break;
        case PINKY:  newX = 9; newY = 9; break;
        case INKY:   newX = 10; newY = 9; break;
        case CLYDE:  newX = 11; newY = 9; break;
        }

        animateGhostReturn(sdlRen, ren, ghosts[idx], eX, eY, newX, newY);

        ghosts[idx].mode = CHASE;
        ghosts[idx].frightenedTimer = 0;
        ghosts[idx].active = true;
        ghosts[idx].x = ghosts[idx].prevX = newX;
        ghosts[idx].y = ghosts[idx].prevY = newY;
        ghosts[idx].releaseTimer = 0;
        return false;
    }

    // Normal hit – lose a life
    lives--;
    playDeathAnimation(sdlRen, ren);
    return true;
}

// ──────────────────────────────────────────────────────────────────
// READY! screen (shows for 2 seconds)
// ──────────────────────────────────────────────────────────────────
static void showReadyScreen(SDL_Renderer* sdlRen, Renderer& ren) {
    Uint32 start = SDL_GetTicks();
    SDL_Event e;
    while (SDL_GetTicks() - start < 2000) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);
        }
        ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
            score, highScore, lives, level,
            SDL_GetTicks(), true, false, false);
        SDL_Delay(16);
    }
}

// ──────────────────────────────────────────────────────────────────
// GAME OVER screen – returns true if player wants to play again
// ──────────────────────────────────────────────────────────────────
static bool showGameOverScreen(SDL_Renderer* sdlRen, Renderer& ren) {
    SDL_Event e;
    Uint32 start = SDL_GetTicks();
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return false;
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_r) return true;
                if (k == SDLK_q || k == SDLK_ESCAPE) return false;
            }
        }
        ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
            score, highScore, lives, level,
            SDL_GetTicks(), false, true, false);
        SDL_Delay(16);
    }
}

// ──────────────────────────────────────────────────────────────────
// Try to turn Pac-Man in the queued direction
// ──────────────────────────────────────────────────────────────────
static bool canMove(char dir) {
    int nx = pacX, ny = pacY;
    if (dir == 'w') ny--;
    else if (dir == 's') ny++;
    else if (dir == 'a') nx--;
    else if (dir == 'd') nx++;
    if (ny == 9) { if (nx >= COLS) nx = 0; else if (nx < 0) nx = COLS - 1; }
    if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) return false;
    return maze[ny][nx] != '#' && maze[ny][nx] != '=';
}

// ──────────────────────────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────────────────────────
int main(int /*argc*/, char* /*argv*/[]) {
    srand((unsigned)time(nullptr));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "PAC-MAN",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
        SDL_Quit(); return 1;
    }

    SDL_Renderer* sdlRen = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!sdlRen) {
        fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }

    Renderer ren(sdlRen);
    bool keepPlaying = true;

    while (keepPlaying) {
        restartGame();
        showReadyScreen(sdlRen, ren);

        Uint32 lastTick = SDL_GetTicks();
        int    pacMoveAcc = 0;
        int    ghostMoveAcc = 0;
        bool   paused = false;
        bool   running = true;

        while (running && lives > 0) {
            // ── Events ──────────────────────────────────────────
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) { running = false; keepPlaying = false; break; }
                if (e.type == SDL_KEYDOWN) {
                    SDL_Keycode k = e.key.keysym.sym;
                    if (k == SDLK_ESCAPE) { running = false; keepPlaying = false; break; }
                    if (k == SDLK_p) { paused = !paused; }
                    // Movement (WASD or arrows)
                    if (k == SDLK_w || k == SDLK_UP)    queuedDir = 'w';
                    if (k == SDLK_s || k == SDLK_DOWN)  queuedDir = 's';
                    if (k == SDLK_a || k == SDLK_LEFT)  queuedDir = 'a';
                    if (k == SDLK_d || k == SDLK_RIGHT) queuedDir = 'd';
                }
            }
            if (!running) break;
            if (paused) {
                ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
                    score, highScore, lives, level,
                    SDL_GetTicks(), false, false, true);
                SDL_Delay(16);
                lastTick = SDL_GetTicks(); // avoid dt spike on unpause
                continue;
            }

            // ── Delta time ───────────────────────────────────────
            Uint32 now = SDL_GetTicks();
            int dt = (int)(now - lastTick);
            lastTick = now;
            if (dt > 100) dt = 100; // clamp spike

            // ── Pac-Man movement ─────────────────────────────────
            pacMoveAcc += dt;
            if (pacMoveAcc >= PAC_MOVE_INTERVAL) {
                pacMoveAcc = 0;
                prevPacX = pacX; prevPacY = pacY;

                // Try queued direction first (feels responsive like the arcade)
                if (canMove(queuedDir)) currentDir = queuedDir;

                movePacman(pacX, pacY, currentDir, maze, score);

                bool powerPellet = eatPellet(pacX, pacY, maze, score);
                if (powerPellet) frightenGhosts(ghosts, difficulty.frightenedDuration);
            }

            // ── Ghost movement ───────────────────────────────────
            ghostMoveAcc += dt;
            if (ghostMoveAcc >= difficulty.ghostMoveInterval) {
                ghostMoveAcc = 0;
                updateModeCycle(ghosts, now / 100, difficulty);
                updateFrightenedTimers(ghosts);
                updateReleaseTimers(ghosts);
                for (int g = 0; g < 4; g++) {
                    if (ghosts[g].releaseTimer <= 0)
                        moveGhost(ghosts[g], maze, pacX, pacY, currentDir, &ghosts[BLINKY]);
                }
            }

            // ── Collision ────────────────────────────────────────
            int caught = checkGhostCollision();
            if (caught != -1) {
                if (handleCollision(caught, sdlRen, ren)) {
                    // Lost a life
                    if (lives > 0) {
                        respawnPacman();
                        showReadyScreen(sdlRen, ren);
                        lastTick = SDL_GetTicks();
                        pacMoveAcc = ghostMoveAcc = 0;
                    }
                }
            }

            // ── Level complete ───────────────────────────────────
            if (!dotsRemain()) {
                // Phase 1: flash the maze walls 6 times (arcade tradition)
                for (int f = 0; f < 6; f++) {
                    ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
                        score, highScore, lives, level,
                        SDL_GetTicks(), false, false, false, false);
                    SDL_SetRenderDrawBlendMode(sdlRen, SDL_BLENDMODE_BLEND);
                    Uint8 alpha = (f % 2 == 0) ? 140 : 0;
                    SDL_SetRenderDrawColor(sdlRen, 255, 255, 255, alpha);
                    SDL_Rect flashRect = { 0, 0, WIN_W, ROWS * TILE };
                    SDL_RenderFillRect(sdlRen, &flashRect);
                    SDL_SetRenderDrawBlendMode(sdlRen, SDL_BLENDMODE_NONE);
                    SDL_RenderPresent(sdlRen);
                    SDL_Delay(180);
                }

                // Phase 2: show LEVEL CLEAR overlay for ~2 seconds
                {
                    Uint32 lcStart = SDL_GetTicks();
                    SDL_Event e;
                    while (SDL_GetTicks() - lcStart < 2000) {
                        while (SDL_PollEvent(&e)) {
                            if (e.type == SDL_QUIT) { running = false; keepPlaying = false; }
                        }
                        ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
                            score, highScore, lives, level,
                            SDL_GetTicks(), false, false, false, true);
                        SDL_Delay(16);
                    }
                }

                level++;
                initializeMazeWithGhostHouse(maze);
                difficulty = getDifficultyParams(level);
                spawnManager = getSpawnPositions(level);
                initGhosts(ghosts, difficulty);
                respawnPacman();
                showReadyScreen(sdlRen, ren);
                lastTick = SDL_GetTicks();
                pacMoveAcc = ghostMoveAcc = 0;
            }

            // ── Draw ─────────────────────────────────────────────
            ren.drawFrame(maze, pacX, pacY, currentDir, ghosts,
                score, highScore, lives, level,
                now, false, false, false);
        } // inner game loop

        if (score > highScore) highScore = score;

        if (keepPlaying) {
            keepPlaying = showGameOverScreen(sdlRen, ren);
        }
    }

    SDL_DestroyRenderer(sdlRen);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}