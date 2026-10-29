// Renderer.h - draws the city, entities, HUD, minimap and sidebar. READ-ONLY access to Game.
// Concepts: separating drawing from game rules, drawing order (painter's algorithm), culling,
//           sf::VertexArray with caching (minimap).
#ifndef RENDERER_H
#define RENDERER_H

#include "Game.h"
#include "Gfx.h"
#include "Leaderboard.h"

SVec cameraCenter(const Game& w);                              // follows the player, clamped to the map
void drawWorld(Gfx& g, const Game& w, bool debugPaths);        // city + entities (uses the world view)

// The minimap is ONE vertex array that is rebuilt only when the graph's version number changes.
class Minimap {
public:
    void draw(Gfx& g, const Game& w);
private:
    void rebuild(const Graph& graph);
    sf::VertexArray va_{sf::PrimitiveType::Triangles};
    unsigned builtFor_ = ~0u;
};

void drawHud(Gfx& g, const Game& w, Minimap& minimap);         // health, mission, minimap, hotbar, warnings
void drawSidebar(Gfx& g, const Game& w, const Leaderboard& board, bool paused);

#endif
