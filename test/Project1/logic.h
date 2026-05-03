#pragma once
#ifndef LOGIC_H
#define LOGIC_H

const int ROWS = 21;
const int COLS = 21;

void movePacman(int& x, int& y, char dir, char maze[ROWS][COLS + 1], int& score);
bool eatPellet(int x, int y, char maze[ROWS][COLS + 1], int& score);
void resetMaze(char maze[ROWS][COLS + 1]);
void initializeMazeWithGhostHouse(char maze[ROWS][COLS + 1]);

#endif