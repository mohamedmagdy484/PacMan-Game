#include "renderer.h"
#include <cmath>
#include <cstring>
#include <cstdio>

// ──────────────────────────────────────────────────────────────────
// Low-level helpers
// ──────────────────────────────────────────────────────────────────
void Renderer::setColor(Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
}

void Renderer::fillRect(int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(ren, &r);
}

// Mid-point circle – draw filled
void Renderer::fillCircle(int cx, int cy, int radius) {
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)sqrt((double)(radius * radius - dy * dy));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Renderer::drawCircle(int cx, int cy, int radius) {
    int x = radius, y = 0;
    int err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(ren, cx + x, cy + y);
        SDL_RenderDrawPoint(ren, cx + y, cy + x);
        SDL_RenderDrawPoint(ren, cx - y, cy + x);
        SDL_RenderDrawPoint(ren, cx - x, cy + y);
        SDL_RenderDrawPoint(ren, cx - x, cy - y);
        SDL_RenderDrawPoint(ren, cx - y, cy - x);
        SDL_RenderDrawPoint(ren, cx + y, cy - x);
        SDL_RenderDrawPoint(ren, cx + x, cy - y);
        y++;
        if (err <= 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

// ──────────────────────────────────────────────────────────────────
// Bitmap font – 5×7 pixel characters (digits, letters, symbols)
// Each character is 5 columns × 7 rows, stored as 7 bytes, bit7=col0
// ──────────────────────────────────────────────────────────────────
static const Uint8 FONT5x7[][7] = {
    // ' '(32)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // '!'
    {0x04,0x04,0x04,0x04,0x00,0x04,0x00},
    // '"'
    {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00},
    // '#'
    {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00},
    // '$'
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04},
    // '%'
    {0x18,0x19,0x02,0x04,0x13,0x03,0x00},
    // '&'
    {0x08,0x14,0x14,0x08,0x15,0x12,0x0D},
    // '\''
    {0x04,0x04,0x00,0x00,0x00,0x00,0x00},
    // '('
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02},
    // ')'
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08},
    // '*'
    {0x00,0x04,0x15,0x0E,0x15,0x04,0x00},
    // '+'
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},
    // ','
    {0x00,0x00,0x00,0x00,0x04,0x04,0x08},
    // '-'
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    // '.'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C},
    // '/'
    {0x00,0x01,0x02,0x04,0x08,0x10,0x00},
    // '0'
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    // '1'
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    // '2'
    {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    // '3'
    {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
    // '4'
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    // '5'
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    // '6'
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    // '7'
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    // '8'
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    // '9'
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    // ':'
    {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00},
    // ';'
    {0x00,0x00,0x04,0x00,0x04,0x04,0x08},
    // '<'
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02},
    // '='
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
    // '>'
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08},
    // '?'
    {0x0E,0x11,0x01,0x02,0x04,0x00,0x04},
    // '@' – unused filler
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E},
    // 'A'
    {0x04,0x0A,0x11,0x11,0x1F,0x11,0x11},
    // 'B'
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    // 'C'
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    // 'D'
    {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    // 'E'
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    // 'F'
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    // 'G'
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    // 'H'
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    // 'I'
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    // 'J'
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    // 'K'
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    // 'L'
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    // 'M'
    {0x11,0x1B,0x15,0x11,0x11,0x11,0x11},
    // 'N'
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    // 'O'
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    // 'P'
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    // 'Q'
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    // 'R'
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    // 'S'
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    // 'T'
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    // 'U'
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    // 'V'
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    // 'W'
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    // 'X'
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    // 'Y'
    {0x11,0x11,0x11,0x0A,0x04,0x04,0x04},
    // 'Z'
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
};

void Renderer::drawDigit(char c, int x, int y, int w, int h, Color col) {
    int idx = -1;
    if (c >= ' ' && c <= 'Z') idx = c - ' ';
    if (idx < 0) return;

    const Uint8* glyph = FONT5x7[idx];
    float pw = w / 5.0f;
    float ph = h / 7.0f;

    setColor(col);
    for (int row = 0; row < 7; row++) {
        for (int bit = 0; bit < 5; bit++) {
            if (glyph[row] & (0x10 >> bit)) {
                int px = x + (int)(bit * pw);
                int py = y + (int)(row * ph);
                int pw2 = (int)(pw)+1;
                int ph2 = (int)(ph)+1;
                fillRect(px, py, pw2, ph2);
            }
        }
    }
}

void Renderer::drawText(const char* text, int x, int y, int charW, int charH, Color col) {
    int cx = x;
    for (int i = 0; text[i]; i++) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') c = c - 'a' + 'A'; // upper-case
        drawDigit(c, cx, y, charW, charH, col);
        cx += charW + 2;
    }
}

// ──────────────────────────────────────────────────────────────────
// Wall drawing – arcade-accurate thin border style
//
// Each wall tile draws blue border segments only on the sides that
// face open space (non-wall neighbours).  Concave inner corners
// (where two open edges meet at a wall tile) get a rounded arc,
// exactly like the original arcade cabinet.
// ──────────────────────────────────────────────────────────────────
static bool isWall(char maze[ROWS][COLS + 1], int col, int row) {
    if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return true; // treat OOB as wall
    return maze[row][col] == '#';
}

// Draw a quarter-circle arc (outline only) at (acx,acy) with given radius,
// covering the quadrant indicated by (qx,qy) signs: qx=-1 means left half, etc.
static void drawArcQuadrant(SDL_Renderer* r, int acx, int acy, int radius, int qx, int qy) {
    // Midpoint circle restricted to one quadrant
    int x = radius, y = 0, err = 0;
    while (x >= y) {
        // The eight octant points – filter to just the target quadrant
        auto put = [&](int px, int py) {
            if (qx * px >= 0 && qy * py >= 0)
                SDL_RenderDrawPoint(r, acx + px, acy + py);
            };
        put(x, y); put(y, x);
        put(-y, x); put(-x, y);
        put(-x, -y); put(-y, -x);
        put(y, -x); put(x, -y);
        y++;
        if (err <= 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void Renderer::drawWall(int col, int row, char maze[ROWS][COLS + 1]) {
    const int px = col * TILE;
    const int py = row * TILE;
    const int W = TILE;
    const int H = TILE;

    // Border thickness (pixels) and arc radius for concave corners
    const int T = 3;   // line thickness
    const int R = 6;   // corner arc radius (must be > T)

    // Neighbour wall flags
    bool U = isWall(maze, col, row - 1);
    bool D = isWall(maze, col, row + 1);
    bool L = isWall(maze, col - 1, row);
    bool Ri = isWall(maze, col + 1, row);
    // Diagonal neighbours (needed for convex outer corners)
    bool UL = isWall(maze, col - 1, row - 1);
    bool UR = isWall(maze, col + 1, row - 1);
    bool DL = isWall(maze, col - 1, row + 1);
    bool DR = isWall(maze, col + 1, row + 1);

    // Fill tile black
    setColor(C_BLACK);
    fillRect(px, py, W, H);

    // ── Border edges ──────────────────────────────────────────────
    // We draw a filled rectangle of thickness T along each face that
    // borders open space (i.e. the neighbour is NOT a wall).
    setColor(C_WALL);

    if (!U)  fillRect(px, py, W, T);          // top edge
    if (!D)  fillRect(px, py + H - T, W, T);          // bottom edge
    if (!L)  fillRect(px, py, T, H);          // left edge
    if (!Ri) fillRect(px + W - T, py, T, H);          // right edge

    // ── Concave inner corners (arc where two open edges meet) ─────
    // When both the top and left neighbour are walls but the top-left
    // diagonal is open, we have a convex turn — no special treatment.
    // When one axis-neighbour is open AND the other axis-neighbour is
    // open, we carve a rounded concave arc at the interior corner.

    // Top-left concave: both U and L are open
    if (!U && !L) {
        // The inner corner point is (px, py) — top-left of tile.
        // Draw a rounded arc centred at (px+R, py+R).
        setColor(C_BLACK);
        // Flood the corner square so the straight edges don't overdraw
        fillRect(px, py, R + T, R + T);
        // Draw arc
        setColor(C_WALL);
        for (int t = 0; t < T; t++)
            drawArcQuadrant(ren, px + R, py + R, R - t, -1, -1);
    }

    // Top-right concave: both U and Ri are open
    if (!U && !Ri) {
        setColor(C_BLACK);
        fillRect(px + W - (R + T), py, R + T, R + T);
        setColor(C_WALL);
        for (int t = 0; t < T; t++)
            drawArcQuadrant(ren, px + W - R, py + R, R - t, 1, -1);
    }

    // Bottom-left concave: both D and L are open
    if (!D && !L) {
        setColor(C_BLACK);
        fillRect(px, py + H - (R + T), R + T, R + T);
        setColor(C_WALL);
        for (int t = 0; t < T; t++)
            drawArcQuadrant(ren, px + R, py + H - R, R - t, -1, 1);
    }

    // Bottom-right concave: both D and Ri are open
    if (!D && !Ri) {
        setColor(C_BLACK);
        fillRect(px + W - (R + T), py + H - (R + T), R + T, R + T);
        setColor(C_WALL);
        for (int t = 0; t < T; t++)
            drawArcQuadrant(ren, px + W - R, py + H - R, R - t, 1, 1);
    }

    // ── Convex outer corners ──────────────────────────────────────
    // When both axis-neighbours are walls but the diagonal is open,
    // a small square notch is left uncovered — fill it with wall colour.
    if (U && L && !UL) fillRect(px, py, T, T);
    if (U && Ri && !UR) fillRect(px + W - T, py, T, T);
    if (D && L && !DL) fillRect(px, py + H - T, T, T);
    if (D && Ri && !DR) fillRect(px + W - T, py + H - T, T, T);
}

// ──────────────────────────────────────────────────────────────────
// Pellets
// ──────────────────────────────────────────────────────────────────
void Renderer::drawDot(int col, int row) {
    int cx = col * TILE + TILE / 2;
    int cy = row * TILE + TILE / 2;
    setColor(C_DOT);
    fillRect(cx - 2, cy - 2, 4, 4);
}

void Renderer::drawPowerPellet(int col, int row, Uint32 nowMs) {
    if ((nowMs / 300) % 2 == 0) return; // blink off
    int cx = col * TILE + TILE / 2;
    int cy = row * TILE + TILE / 2;
    setColor(C_POWERPELLET);
    fillCircle(cx, cy, TILE / 4);
}

// ──────────────────────────────────────────────────────────────────
// Pac-Man  –  pie-chart wedge, mouth animates open/close
// ──────────────────────────────────────────────────────────────────
void Renderer::drawPacman(int col, int row, char dir, Uint32 nowMs) {
    int cx = col * TILE + TILE / 2;
    int cy = row * TILE + TILE / 2;
    int R = TILE / 2 - 1;

    // Mouth angle oscillates 0..40 degrees
    float mouthMax = 40.0f * 3.14159f / 180.0f;
    float phase = (nowMs % 400) / 400.0f;
    float mouthAngle = (phase < 0.5f) ? (phase * 2 * mouthMax) : ((1 - phase) * 2 * mouthMax);

    // Direction offset in radians
    float baseAngle = 0.0f;
    if (dir == 'a') baseAngle = 3.14159f;
    else if (dir == 'w') baseAngle = -3.14159f / 2;
    else if (dir == 's') baseAngle = 3.14159f / 2;
    else baseAngle = 0.0f; // 'd' / default

    setColor(C_YELLOW);
    // Draw filled pie excluding the mouth wedge
    int steps = 64;
    float startAngle = baseAngle + mouthAngle;
    float endAngle = baseAngle + 2 * 3.14159f - mouthAngle;
    float step = (endAngle - startAngle) / steps;

    for (int i = 0; i < steps; i++) {
        float a0 = startAngle + i * step;
        float a1 = startAngle + (i + 1) * step;
        // Filled triangle: centre → arc0 → arc1
        int x0 = cx + (int)(R * cos(a0));
        int y0 = cy + (int)(R * sin(a0));
        int x1 = cx + (int)(R * cos(a1));
        int y1 = cy + (int)(R * sin(a1));
        // Use SDL_RenderDrawLine spokes for dense fill
        SDL_RenderDrawLine(ren, cx, cy, x0, y0);
        SDL_RenderDrawLine(ren, cx, cy, x1, y1);
    }
    // Dense fill with horizontal lines
    for (int dy = -R; dy <= R; dy++) {
        float halfW = sqrt((float)(R * R - dy * dy));
        int lx = cx - (int)halfW;
        int rx = cx + (int)halfW;
        for (int x = lx; x <= rx; x++) {
            float angle = atan2((float)dy, (float)(x - cx));
            // normalise angle to [baseAngle, baseAngle+2pi]
            float rel = angle - baseAngle;
            while (rel < 0)           rel += 2 * 3.14159f;
            while (rel > 2 * 3.14159f) rel -= 2 * 3.14159f;
            if (rel > mouthAngle && rel < 2 * 3.14159f - mouthAngle) {
                SDL_RenderDrawPoint(ren, x, cy + dy);
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────
// Ghost  –  rounded body + skirt + eyes
// ──────────────────────────────────────────────────────────────────
void Renderer::drawGhostEyes(int cx, int cy, char dir) {
    // Two white eye sclera
    int offX = 4, offY = -2;
    setColor(C_WHITE);
    fillCircle(cx - offX, cy + offY, 3);
    fillCircle(cx + offX, cy + offY, 3);

    // Pupils move with direction
    int px = 0, py = 0;
    if (dir == 'a') px = -2;
    else if (dir == 'd') px = 2;
    else if (dir == 'w') py = -2;
    else if (dir == 's') py = 2;

    Color blue = { 0,  0, 180, 255 };
    setColor(blue);
    fillCircle(cx - offX + px, cy + offY + py, 2);
    fillCircle(cx + offX + px, cy + offY + py, 2);
}

void Renderer::drawGhostBody(int cx, int cy, Color bodyColor) {
    int R = TILE / 2 - 1;
    setColor(bodyColor);

    // Filled semicircle on top
    for (int dy = -R; dy <= 0; dy++) {
        int hw = (int)sqrt((float)(R * R - dy * dy));
        fillRect(cx - hw, cy + dy, hw * 2, 1);
    }

    // Rectangle body
    fillRect(cx - R, cy, R * 2, R);

    // Skirt – 3 bumps at the bottom (zigzag)
    int bumpW = (R * 2) / 3;
    int bumpH = TILE / 5;
    int bY = cy + R;
    for (int b = 0; b < 3; b++) {
        int bX = cx - R + b * bumpW;
        fillCircle(bX + bumpW / 2, bY, bumpW / 2);
    }

    // Carve black triangles between bumps
    setColor(C_BLACK);
    fillRect(cx - R + bumpW - 2, bY, 4, bumpH);
    fillRect(cx - R + 2 * bumpW - 2, bY, 4, bumpH);
}

void Renderer::drawGhost(const Ghost& g, Uint32 nowMs) {
    int cx = g.x * TILE + TILE / 2;
    int cy = g.y * TILE + TILE / 2;

    if (g.mode == FRIGHTENED) {
        bool flashing = g.frightenedTimer < 20 && (nowMs / 200) % 2 == 0;
        Color body = flashing ? C_FLASH : C_FRIGHTENED;
        drawGhostBody(cx, cy, body);

        // Frightened eyes – two dots and wavy mouth in white
        setColor(C_WHITE);
        fillRect(cx - 5, cy - 3, 3, 3);
        fillRect(cx + 3, cy - 3, 3, 3);
        // Wavy mouth
        for (int i = -5; i <= 5; i++) {
            int my = cy + 3 + (i % 2 == 0 ? 2 : 0);
            SDL_RenderDrawPoint(ren, cx + i, my);
            SDL_RenderDrawPoint(ren, cx + i, my + 1);
        }
        return;
    }

    Color bodyColor;
    switch (g.type) {
    case BLINKY: bodyColor = C_RED;    break;
    case PINKY:  bodyColor = C_PINK;   break;
    case INKY:   bodyColor = C_CYAN;   break;
    case CLYDE:  bodyColor = C_ORANGE; break;
    default:     bodyColor = C_WHITE;  break;
    }

    drawGhostBody(cx, cy, bodyColor);
    drawGhostEyes(cx, cy - TILE / 6, g.prevDir);
}

// ──────────────────────────────────────────────────────────────────
// HUD
// ──────────────────────────────────────────────────────────────────
void Renderer::drawHUD(int score, int highScore, int lives, int level) {
    int y = ROWS * TILE;
    setColor(C_BLACK);
    fillRect(0, y, WIN_W, HUD_H);

    char buf[64];

    // Score label + value
    drawText("SCORE", 4, y + 6, 7, 10, C_WHITE);
    snprintf(buf, sizeof(buf), "%d", score);
    drawText(buf, 4, y + 20, 7, 10, C_YELLOW);

    // High score
    drawText("HIGH", WIN_W / 2 - 28, y + 6, 7, 10, C_WHITE);
    snprintf(buf, sizeof(buf), "%d", highScore);
    drawText(buf, WIN_W / 2 - 28, y + 20, 7, 10, C_YELLOW);

    // Level
    snprintf(buf, sizeof(buf), "LVL %d", level);
    drawText(buf, WIN_W - 70, y + 6, 7, 10, C_WHITE);

    // Lives – draw small Pac-Man icons
    for (int i = 0; i < lives && i < 5; i++) {
        int lx = 6 + i * 18;
        int ly = y + 40;
        // Mini pac-man: yellow filled circle with notch
        setColor(C_YELLOW);
        fillCircle(lx + 6, ly + 6, 6);
        setColor(C_BLACK);
        // Mouth wedge
        for (int dy = 0; dy <= 6; dy++)
            SDL_RenderDrawLine(ren, lx + 6, ly + 6, lx + 6 + dy, ly + 6 - dy / 2);
    }
}

// ──────────────────────────────────────────────────────────────────
// Centred text overlay (READY! / GAME OVER)
// ──────────────────────────────────────────────────────────────────
void Renderer::drawOverlay(const char* line1, const char* line2, Color col, const char* line3) {
    int charW = 12, charH = 18;
    int gap = 28;

    // Semi-transparent black backdrop – taller to fit 3 lines
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    fillRect(WIN_W / 2 - 130, ROWS * TILE / 2 - 50, 260, 110);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    int baseY = ROWS * TILE / 2 - 38;

    int len1 = (int)strlen(line1);
    int textW1 = len1 * (charW + 2);
    drawText(line1, WIN_W / 2 - textW1 / 2, baseY, charW, charH, col);

    if (line2 && line2[0]) {
        int len2 = (int)strlen(line2);
        int textW2 = len2 * (charW + 2);
        drawText(line2, WIN_W / 2 - textW2 / 2, baseY + gap, charW, charH, C_WHITE);
    }

    if (line3 && line3[0]) {
        // Smaller font for the hint line
        int cW = 8, cH = 12;
        int len3 = (int)strlen(line3);
        int textW3 = len3 * (cW + 2);
        Color hint = { 200, 200, 200, 255 };
        drawText(line3, WIN_W / 2 - textW3 / 2, baseY + gap * 2 + 4, cW, cH, hint);
    }
}


// ──────────────────────────────────────────────────────────────────
// Level Complete overlay – celebratory panel with stars
// ──────────────────────────────────────────────────────────────────
void Renderer::drawLevelComplete(int level, Uint32 nowMs) {
    // ── Semi-transparent dark backdrop ───────────────────────
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 200);
    fillRect(WIN_W / 2 - 140, ROWS * TILE / 2 - 70, 280, 140);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    // ── Animated border – cycles through arcade colours ──────
    const Color borderCols[4] = { C_YELLOW, C_CYAN, C_PINK, C_ORANGE };
    Color bCol = borderCols[(nowMs / 150) % 4];
    setColor(bCol);
    // Top & bottom bars
    fillRect(WIN_W / 2 - 140, ROWS * TILE / 2 - 70, 280, 3);
    fillRect(WIN_W / 2 - 140, ROWS * TILE / 2 + 67, 280, 3);
    // Left & right bars
    fillRect(WIN_W / 2 - 140, ROWS * TILE / 2 - 70, 3, 140);
    fillRect(WIN_W / 2 + 137, ROWS * TILE / 2 - 70, 3, 140);

    // ── "LEVEL CLEAR" title ───────────────────────────────────
    int charW = 13, charH = 20;
    const char* title = "LEVEL  CLEAR";
    int titleW = (int)strlen(title) * (charW + 2);
    // Alternating yellow/white flash on the title
    Color titleCol = ((nowMs / 300) % 2 == 0) ? C_YELLOW : C_WHITE;
    drawText(title, WIN_W / 2 - titleW / 2, ROWS * TILE / 2 - 58, charW, charH, titleCol);

    // ── Level number ──────────────────────────────────────────
    char lvlBuf[16];
    snprintf(lvlBuf, sizeof(lvlBuf), "STAGE  %d", level);
    int lvlW = (int)strlen(lvlBuf) * (10 + 2);
    drawText(lvlBuf, WIN_W / 2 - lvlW / 2, ROWS * TILE / 2 - 28, 10, 15, C_CYAN);

    // ── Animated stars – 7 stars placed in a horizontal row ──
    // Each star pulses in size using a sine wave offset per star
    for (int s = 0; s < 7; s++) {
        float phase = (nowMs / 200.0f) + s * 0.9f;
        float pulse = 0.5f + 0.5f * sinf(phase); // 0.0 – 1.0
        int   r = 2 + (int)(pulse * 4);       // radius 2–6 px

        int starX = WIN_W / 2 - 70 + s * 20;
        int starY = ROWS * TILE / 2 + 18;

        // Alternate star colours
        const Color starCols[3] = { C_YELLOW, C_ORANGE, C_WHITE };
        setColor(starCols[s % 3]);

        // Draw a simple 4-point star: two crossing lines + diagonals
        // Horizontal spoke
        fillRect(starX - r, starY - 1, r * 2 + 1, 3);
        // Vertical spoke
        fillRect(starX - 1, starY - r, 3, r * 2 + 1);
        // Diagonal spokes (half length)
        int d = r / 2;
        for (int p = -d; p <= d; p++) {
            SDL_RenderDrawPoint(ren, starX + p, starY + p);
            SDL_RenderDrawPoint(ren, starX + p, starY + p + 1);
            SDL_RenderDrawPoint(ren, starX + p, starY - p);
            SDL_RenderDrawPoint(ren, starX + p, starY - p + 1);
        }
    }

    // ── "GET READY" hint at the bottom ───────────────────────
    const char* hint = "GET  READY";
    int hintW = (int)strlen(hint) * (8 + 2);
    Color hintCol = { 180, 180, 180, 255 };
    drawText(hint, WIN_W / 2 - hintW / 2, ROWS * TILE / 2 + 42, 8, 12, hintCol);
}

// ──────────────────────────────────────────────────────────────────
// Master draw
// ──────────────────────────────────────────────────────────────────
void Renderer::drawFrame(
    char maze[ROWS][COLS + 1],
    int pacX, int pacY, char pacDir,
    const Ghost ghosts[4],
    int score, int highScore,
    int lives, int level,
    Uint32 nowMs,
    bool showReady,
    bool showGameOver,
    bool paused,
    bool showLevelComplete)
{
    // Clear
    setColor(C_BLACK);
    SDL_RenderClear(ren);

    // Draw maze tiles
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            char tile = maze[row][col];
            switch (tile) {
            case '#':
                drawWall(col, row, maze);
                break;
            case '.':
                drawDot(col, row);
                break;
            case 'o':
                drawPowerPellet(col, row, nowMs);
                break;
            case '=': {
                // Ghost house door – pink horizontal bar
                int px = col * TILE, py = row * TILE;
                setColor(C_BLACK); fillRect(px, py, TILE, TILE);
                setColor(C_DOOR);
                fillRect(px, py + TILE / 2 - 2, TILE, 4);
                break;
            }
            default:
                // empty / ' '
                setColor(C_BLACK);
                fillRect(col * TILE, row * TILE, TILE, TILE);
                break;
            }
        }
    }

    // Pac-Man
    drawPacman(pacX, pacY, pacDir, nowMs);

    // Ghosts
    for (int g = 0; g < 4; g++) {
        if (ghosts[g].active) drawGhost(ghosts[g], nowMs);
    }

    // HUD
    drawHUD(score, highScore, lives, level);

    // Overlays
    if (showReady) {
        drawOverlay("READY!", "", { 255, 255, 0, 255 });
    }
    if (showGameOver) {
        char scoreBuf[32];
        snprintf(scoreBuf, sizeof(scoreBuf), "SCORE  %d", score);
        drawOverlay("GAME  OVER", scoreBuf, C_RED, "R=RESTART   Q=QUIT");
    }
    if (paused) {
        drawOverlay("PAUSED", "PRESS P", C_WHITE);
    }
    if (showLevelComplete) {
        drawLevelComplete(level, nowMs);
    }

    SDL_RenderPresent(ren);
}