#ifndef ENTITIES_H
#define ENTITIES_H

#include <vector>
#include "GameTypes.h"

struct Zombie {
    Vec2 pos;
    ZombieKind kind = ZombieKind::Walker;
    float radius = 12.f, hp = 3.f, maxHp = 3.f, speed = 70.f, damage = 15.f, threat = 1.f;
    int points = 100;
    bool alive = true;
    float repath = 0.f, jitter = 0.f;
    float alert = 0.f;                  
    float shootTimer = 1.5f;            
    float abilityTimer = 4.f;           
    float charging = 0.f;               
    Vec2i attackTile{-1, -1};           
    std::vector<Vec2i> path;            
};

struct Pickup { PickupKind kind; Vec2 pos; bool alive = true; };
struct Bullet { Vec2 pos, vel; float life; int damage; bool alive; };
struct Acid { Vec2 pos, vel; float life; bool alive; };          
struct Grenade { Vec2 pos, target; float fuse; bool alive; };
struct Explosion { Vec2 pos; float t; };

struct PathStat {
    double avgUs = 0.0;
    long calls = 0;
    void add(double us) { avgUs = calls == 0 ? us : 0.95 * avgUs + 0.05 * us; ++calls; }
};

struct Input {
    Vec2 move;
    Vec2 aim = Vec2(1.f, 0.f);
    bool fire = false, sprint = false;
};

enum class Status { Playing, MissionClear, Won, Lost };

inline const char* kindName(ZombieKind k) {
    switch (k) {
    case ZombieKind::Walker: return "Walker";
    case ZombieKind::Runner: return "Runner";
    case ZombieKind::Tank: return "Tank";
    case ZombieKind::Spitter: return "Spitter";
    case ZombieKind::Boss: return "BOSS";
    }
    return "?";
}
inline const char* diffName(Difficulty d) { return d == Difficulty::Easy ? "EASY" : d == Difficulty::Normal ? "NORMAL" : "HARD"; }
inline const char* objectiveText(int i) {
    static const char* const t[3] = {"Kill 10 zombies", "Collect 3 pickups", "Place 2 barricades"};
    return t[i < 0 ? 0 : i > 2 ? 2 : i];
}

#endif
