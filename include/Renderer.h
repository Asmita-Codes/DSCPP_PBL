#ifndef RENDERER_H
#define RENDERER_H

#include "Game.h"
#include "Gfx.h"
#include "Leaderboard.h"

SVec cameraCenter(const Game& w);                              
void drawWorld(Gfx& g, const Game& w, bool debugPaths);        


class Minimap {
public:
    void draw(Gfx& g, const Game& w);
private:
    void rebuild(const Graph& graph);
    sf::VertexArray va_{sf::PrimitiveType::Triangles};
    unsigned builtFor_ = ~0u;
};

void drawHud(Gfx& g, const Game& w, Minimap& minimap);         
void drawSidebar(Gfx& g, const Game& w, const Leaderboard& board, bool paused);

#endif
