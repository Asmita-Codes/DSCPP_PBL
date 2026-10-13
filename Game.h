#ifndef GAME_H
#define GAME_H

#include <algorithm>
#include <random>
#include <string>
#include <vector>
#include "ActionStack.h"
#include "Entities.h"
#include "GameTypes.h"
#include "Graph.h"
#include "Inventory.h"
#include "Mission.h"
#include "PriorityQueue.h"
#include "Queue.h"

class Game {
public:
    Game();

   
    Graph graph;               
    Inventory inventory;       
    Queue waves;               
    PriorityQueue threats;     
    ActionStack undo;          
    MissionManager mission;    


    std::vector<Zombie> zombies;
    std::vector<Pickup> pickups;
    std::vector<Bullet> bullets;
    std::vector<Acid> acids;
    std::vector<Grenade> grenades;
    std::vector<Explosion> explosions;
    std::vector<Vec2i> route;            


    Vec2 playerPos;
    float hp = 100.f, stamina = 100.f;
    bool exhausted = false;
    int score = 0, kills = 0;                         
    int missionKills = 0, pickupsCollected = 0, barricadesPlaced = 0;   
    unsigned objectivesDone = 0;                     
    int weapon = 0;                                    
    bool flashlight = true;
    Vec2 aimDir = Vec2(1.f, 0.f);
    int combo = 0;                                     
    float comboTimer = 0.f;


    Status status = Status::Playing;
    Difficulty difficulty = Difficulty::Normal;
    float gameTime = 0.f, missionTime = 0.f, waveTimer = 0.f, airDropTimer = 0.f, routeTimer = 0.f;
    float fireCd = 0.f, damageFlash = 0.f, bannerTimer = 0.f;
    int waveNumber = 0;
    std::string banner;


    bool useDijkstra = true;
    PathStat bfsStat, dijStat;
    float threatDistance = -1.f;
    ZombieKind threatKind = ZombieKind::Walker;

    std::mt19937 rng;

    static constexpr float BARRICADE_HP = 8.f;

  
    void newGame(Difficulty d);         
    void nextMission();                 
    void update(float dt, const Input& in);

  
    bool toggleBarricade(Vec2 worldPos); 
    bool undoBarricade();
    bool useMedkit();
    bool throwGrenade(Vec2 target);


    void invalidatePaths();
    void showBanner(const std::string& s, float seconds = 2.5f) { banner = s; bannerTimer = seconds; }
    int comboMultiplier() const { return std::min(5, 1 + combo / 3); }
    float barricadeFraction(Vec2i t) const;         
    float waveInterval() const;
    bool routeTarget(Vec2i& out) const;              
    int zombieCount(ZombieKind k) const;

    SaveData toSave() const;
    void fromSave(const SaveData& d);

private:
    float urand() { return std::uniform_real_distribution<float>(0.f, 1.f)(rng); }
    float diffSpeed() const { return difficulty == Difficulty::Easy ? 0.8f : difficulty == Difficulty::Hard ? 1.15f : 1.f; }
    float diffDamage() const { return difficulty == Difficulty::Easy ? 0.7f : difficulty == Difficulty::Hard ? 1.3f : 1.f; }
    float diffWave() const { return difficulty == Difficulty::Easy ? 0.75f : difficulty == Difficulty::Hard ? 1.3f : 1.f; }

    void startMission(int index);
    void completeMission();
    Vec2i randomRoadTile(float minDistFromPlayer);
    std::vector<Vec2i> findPath(Vec2i a, Vec2i b);
    bool tileOccupied(Vec2i t) const;

    void makeZombie(ZombieKind kind, Vec2 pos, float jitter);
    ZombieKind rollKind();
    void spawnWave(int baseCount);
    void damageZombie(Zombie& z, float dmg);
    void registerKill(int points);
    void makeNoise(Vec2 at, float radius);
    void explode(Vec2 at);
    void checkObjectives();
    void airDrop();

    void updateZombie(Zombie& z, float dt);
    void moveAlongPath(Zombie& z, float speed, float dt);
    void planBarricadeAttack(Zombie& z);
    void damageBarricade(Vec2i t, float dmg);

    std::vector<float> barricadeHp_;     
};

#endif
