#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>
#include "logic.h"
#include "ghost.h"

// ──────────────────────────────────────────────────────────────────
// Tile / window sizing
// ──────────────────────────────────────────────────────────────────
static const int TILE = 24;          // pixels per maze cell
static const int HUD_H = 64;          // pixels reserved for HUD below maze
static const int WIN_W = COLS * TILE; // 504
static const int WIN_H = ROWS * TILE + HUD_H; // 568

// ──────────────────────────────────────────────────────────────────
// Colours  (0xAARRGGBB ready for SDL_SetRenderDrawColor)
// ──────────────────────────────────────────────────────────────────
struct Color { Uint8 r, g, b, a; };

static const Color C_BLACK = { 0,   0,   0, 255 };
static const Color C_WALL = { 33,  33, 222, 255 };  // classic arcade blue
static const Color C_WALL_DARK = { 0,   0, 139, 255 };
static const Color C_DOT = { 255, 184, 151, 255 };  // warm pellet colour
static const Color C_POWERPELLET = { 255, 255, 255, 255 };
static const Color C_YELLOW = { 255, 255,   0, 255 };
static const Color C_RED = { 255,   0,   0, 255 };
static const Color C_PINK = { 255, 184, 255, 255 };
static const Color C_CYAN = { 0, 255, 255, 255 };
static const Color C_ORANGE = { 255, 184,  82, 255 };
static const Color C_FRIGHTENED = { 33,  33, 255, 255 };
static const Color C_FLASH = { 255, 255, 255, 255 };
static const Color C_DOOR = { 255, 184, 255, 255 };  // pink ghost-house door
static const Color C_WHITE = { 255, 255, 255, 255 };
static const Color C_DARKBLUE = { 21,  21,  21, 255 };

// ──────────────────────────────────────────────────────────────────
// Renderer interface
// ──────────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer(SDL_Renderer* r) : ren(r) {}

    // Draw the entire frame
    void drawFrame(
        char maze[ROWS][COLS + 1],
        int pacX, int pacY, char pacDir,
        const Ghost ghosts[4],
        int score, int highScore,
        int lives, int level,
        Uint32 nowMs,
        bool showReady,
        bool showGameOver,
        bool paused,
        bool showLevelComplete = false
    );

private:
    SDL_Renderer* ren;

    void setColor(Color c);
    void fillRect(int x, int y, int w, int h);
    void drawCircle(int cx, int cy, int r);
    void fillCircle(int cx, int cy, int r);

    void drawWall(int col, int row, char maze[ROWS][COLS + 1]);
    void drawDot(int col, int row);
    void drawPowerPellet(int col, int row, Uint32 nowMs);
    void drawPacman(int col, int row, char dir, Uint32 nowMs);
    void drawGhost(const Ghost& g, Uint32 nowMs);
    void drawGhostEyes(int cx, int cy, char dir);
    void drawGhostBody(int cx, int cy, Color bodyColor);
    void drawHUD(int score, int highScore, int lives, int level);
    void drawText(const char* text, int x, int y, int charW, int charH, Color col);
    void drawDigit(char c, int x, int y, int w, int h, Color col);
    void drawOverlay(const char* line1, const char* line2, Color col, const char* line3 = "");
    void drawLevelComplete(int level, Uint32 nowMs);
};

#endif