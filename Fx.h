#ifndef FX_H
#define FX_H

#include <random>
#include <string>
#include <vector>
#include "Audio.h"
#include "Game.h"
#include "Gfx.h"

enum class PType { Blood, Spark, Smoke, Ember, Shell, Chip, Dust, Glint, Acid };

struct Particle {
    PType type;
    Vec2 p, v;
    float life, maxLife, size, grow, rot, spin, drag;
    Col c;
};
enum class DType { Blood, Drop, Scorch, Corpse, Shell, Acid };
struct Decal { DType type; Vec2 p; float rot, size, age, life; int var; };
struct Popup { Vec2 p; std::string s; Col c; float t; };

struct ZVis { Vec2 pos; ZombieKind kind; float hp, heading, phase, flash, age, charging; };   
struct SoundCue { Sfx sfx; Vec2 pos; float vol, pitch; };   
struct PickVis { Vec2 pos; PickupKind kind; float age; };
struct GrenVis { Vec2 target; float startDist; };

class Fx {
public:
    void reset(const Game& w);                        
    void update(const Game& w, float dt, bool observe);   

    Vec2 shakeOffset(double time) const;

    std::vector<Particle> parts;
    std::vector<Decal> decals;
    std::vector<Popup> popups;
    std::vector<ZVis> zv;            
    std::vector<PickVis> pv;         
    std::vector<GrenVis> gv;         
    std::vector<SoundCue> cues;      

    float shake = 0.f, muzzle = 0.f, recoil = 0.f, hurt = 0.f;
    float walkPhase = 0.f, moveAmt = 0.f;
    // co-op: the same for player 2
    float muzzle2 = 0.f, recoil2 = 0.f, hurt2 = 0.f, walkPhase2 = 0.f, moveAmt2 = 0.f;

private:
    float rnd(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng_); }
    void emit(PType t, Vec2 p, Vec2 v, float life, float size, Col c, float grow = 0.f, float drag = 3.f, float spin = 0.f);
    void decal(DType t, Vec2 p, float size, float life, int var = 0, float rot = -1.f);
    void bloodBurst(Vec2 p, Vec2 dir, int n, float force);
    void explosion(Vec2 p, const Game& w);
    void snapshot(const Game& w);
    void cue(Sfx s, Vec2 p, float vol = 1.f, float pitch = 1.f) { if (cues.size() < 64) cues.push_back(SoundCue{s, p, vol, pitch}); }

    std::mt19937 rng_{1234u};
    Vec2 prevPlayer_, prevP2_;
    float prevHp_ = 100.f, hurtAccum_ = 0.f, prevHp2_ = 100.f, hurtAccum2_ = 0.f;
    int prevWeapon2_ = 0;
    int prevKills_ = 0, prevMedkits_ = 0, prevWeapon_ = 0, prevWave_ = 0;
    unsigned prevObjectives_ = 0;
    bool prevFlash_ = true, prevUnlocked_ = false;
    unsigned prevVersion_ = ~0u;
    std::vector<Bullet> prevBullets_;
    std::vector<Acid> prevAcids_;
    std::vector<TileType> prevTiles_;
};

#endif
