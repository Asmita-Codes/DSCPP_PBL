#ifndef ART_H
#define ART_H

#include <vector>
#include "Gfx.h"
#include "Graph.h"

class Art {
public:
    ~Art() { destroy(); }
    bool build(Gfx& g);              
    void destroy();

    
    struct Chunk { SDL_Texture* tex; float x, y, w, h; };
    std::vector<Chunk> ground;

    
    struct Lamp { float x, y; int mode; float seed; };   
    std::vector<Lamp> lamps;

    
    SDL_Texture *glow = nullptr, *soft = nullptr, *cone = nullptr, *vignette = nullptr, *fog = nullptr, *smoke = nullptr;
    SDL_Texture *blood[3] = {}, *scorch = nullptr, *crack = nullptr;
   
    SDL_Texture *rubble[3] = {}, *barricade = nullptr;
    SDL_Texture *player[2][2] = {};  
    SDL_Texture *zombie[5] = {};     
    SDL_Texture *pickup[4] = {};     
   
    SDL_Texture *icoRifle = nullptr, *icoPistol = nullptr, *icoTorch = nullptr, *icoHeart = nullptr, *icoBolt = nullptr;

    
    static float zombieSize(float radius) { return radius * 4.4f; }
    static float playerSize() { return PLAYER_RADIUS * 6.2f; }   
private:
    void bakeGround(Gfx& g);
    std::vector<SDL_Texture*> all_;
};

#endif
