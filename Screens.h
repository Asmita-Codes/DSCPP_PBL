#ifndef SCREENS_H
#define SCREENS_H

#include <string>
#include "Game.h"
#include "Gfx.h"
#include "Leaderboard.h"

enum class Screen { Menu, Leaderboard, Help, Playing, Paused, MissionClear, Won, Lost };

void drawMenu(Gfx& g, bool hasSave, Difficulty diff);
void drawLeaderboardScreen(Gfx& g, const Leaderboard& board);
void drawHelp(Gfx& g);
void drawOverlay(Gfx& g, Screen s, const Game& w);       
void drawToast(Gfx& g, const std::string& message);      

#endif
