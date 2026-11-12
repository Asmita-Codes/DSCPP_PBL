#ifndef SCREENS_H
#define SCREENS_H

#include <string>
#include "Art.h"
#include "Fx.h"
#include "Game.h"
#include "Gfx.h"
#include "Leaderboard.h"
#include "Renderer.h"

enum class Screen { Menu, Multiplayer, Leaderboard, Help, Playing, Paused, MissionClear, Won, Lost };


void drawHud(Gfx& g, const Art& a, WorldRenderer& world, const Game& w, const Fx& fx);
void drawCrosshair(Gfx& g, const Fx& fx);


void drawMenu(Gfx& g, const Art& a, bool canResume, bool hasSave, Difficulty diff, const std::string& musicLabel, bool effectsOn);
void drawLeaderboardScreen(Gfx& g, const Leaderboard& board);
void drawHelp(Gfx& g, const Art& a);
void drawMultiplayer(Gfx& g, const Art& a, bool gamepadConnected, Difficulty diff);   
void drawOverlay(Gfx& g, Screen s, const Game& w);       
void drawToast(Gfx& g, const std::string& message, float timeLeft);

#endif
