#include "logic.h"
#include <iostream>
using namespace std;

void movePacman(int& x, int& y, char dir, char maze[ROWS][COLS + 1], int& score) {
    int nx = x, ny = y;

    if (dir == 'w') ny--;
    else if (dir == 's') ny++;
    else if (dir == 'a') nx--;
    else if (dir == 'd') nx++;

    // Portal teleport (row 9)
    if (ny == 9) {
        if (nx >= COLS) nx = 0;
        else if (nx < 0) nx = COLS - 1;
    }

    if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) return;
    if (maze[ny][nx] == '#') return;
    if (maze[ny][nx] == '=') return;

    x = nx;
    y = ny;
}

bool eatPellet(int x, int y, char maze[ROWS][COLS + 1], int& score) {
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return false;

    if (maze[y][x] == '.') {
        maze[y][x] = ' ';
        score += 10;
        return false;
    }
    if (maze[y][x] == 'o') {
        maze[y][x] = ' ';
        score += 100;
        return true; // power pellet eaten
    }
    return false;
}

void resetMaze(char maze[ROWS][COLS + 1]) {
    const char originalMaze[ROWS][COLS + 1] = {
        " ################### ",
        " #........#........# ",
        " #o##.###.#.###.##o# ",
        " #.................# ",
        " #.##.#.#####.#.##.# ",
        " #....#...#...#....# ",
        " ####.### # ###.#### ",
        "    #.#   0   #.#    ",
        "#####.# ##=## #.#####",
        "     .  #123#  .     ",
        "#####.# ##### #.#####",
        "    #.#       #.#    ",
        " ####.# ##### #.#### ",
        " #........#........# ",
        " #.##.###.#.###.##.# ",
        " #o.#.....P.....#.o# ",
        " ##.#.#.#####.#.#.## ",
        " #....#...#...#....# ",
        " #.######.#.######.# ",
        " #.................# ",
        " ################### "
    };
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j <= COLS; j++)
            maze[i][j] = originalMaze[i][j];
}

void initializeMazeWithGhostHouse(char maze[ROWS][COLS + 1]) {
    resetMaze(maze);
    maze[8][10] = '=';
}