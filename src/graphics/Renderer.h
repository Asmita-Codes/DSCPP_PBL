#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "Art.h"
#include "Fx.h"
#include "Game.h"
#include "Gfx.h"

class WorldRenderer {
public:
    ~WorldRenderer();
    bool init(Gfx& g);                         
    void resetCamera(const Game& w);
    void updateCamera(const Game& w, float dt);
    Vec2 toWorld(FPt ui) const { return Vec2(ui.x + cam_.x, ui.y + cam_.y); }  
    Vec2 camera() const { return cam_; }

    void draw(Gfx& g, const Art& a, const Game& w, const Fx& fx, bool debugPaths);
    void drawBackdrop(Gfx& g, const Art& a);   
    void drawMinimap(Gfx& g, const Game& w, float x, float y, float wd, float ht);

private:
    struct Light { float x, y, r; Col c; };
    void drawGround(Gfx& g, const Art& a, Vec2 tl);
    void lampLights(const Art& a, double t, Vec2 tl);
    void lightPass(Gfx& g, const Art& a, Vec2 tl, Col ambient);   
    void drawSurvivor(Gfx& g, const Art& a, int team, bool tag, Vec2 pos, Vec2 aimDir, int weapon, float walkPhase,
                      float moveAmt, float recoil, float hurt, bool down, float revive);
    struct Cone { Vec2 pos; float ang; };
    std::vector<Cone> cones_;
    Vec2 focus(const Game& w) const;           
    bool visible(Vec2 p, float r, Vec2 tl) const {
        return p.x + r > tl.x && p.x - r < tl.x + UI_W && p.y + r > tl.y && p.y - r < tl.y + UI_H;
    }

    SDL_Texture* light_ = nullptr;
    SDL_Texture* mini_ = nullptr;
    unsigned miniVersion_ = ~0u;
    Vec2 cam_;
    std::vector<Light> lights_;
};

#endif
