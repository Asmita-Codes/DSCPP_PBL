// Game.h - ALL GAME RULES (no SFML, no drawing). The renderer only READS this class.
// What lives here: player movement, shooting, zombie waves and AI (BFS / Dijkstra), special zombies,
// noise, barricades (and zombies chewing through them), grenades, pickups, combo score, campaign
// missions, save/load glue.
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

    // ---- data structures (each in its own file)
    Graph graph;               // Graph.h          city graph + BFS + Dijkstra
    Inventory inventory;       // Inventory.h      linked list
    Queue waves;               // Queue.h          zombie wave queue
    PriorityQueue threats;     // PriorityQueue.h  most dangerous zombie first
    ActionStack undo;          // ActionStack.h    undo history
    MissionManager mission;    // Mission.h        campaign progress

    // ---- entities
    std::vector<Zombie> zombies;
    std::vector<Pickup> pickups;
    std::vector<Bullet> bullets;
    std::vector<Acid> acids;
    std::vector<Grenade> grenades;
    std::vector<Explosion> explosions;
    std::vector<Vec2i> route;             // path from the player to the current goal

    // ---- player + progress
    Vec2 playerPos;
    float hp = 100.f, stamina = 100.f;
    bool exhausted = false;
    int score = 0, kills = 0;                          // whole campaign
    int missionKills = 0, pickupsCollected = 0, barricadesPlaced = 0;   // this mission (bonus objectives)
    unsigned objectivesDone = 0;                       // bit mask of the 3 bonus objectives
    int weapon = 0;                                    // 0 rifle, 1 pistol
    bool flashlight = true;
    Vec2 aimDir = Vec2(1.f, 0.f);
    int combo = 0;                                     // kills in a row (each within 3 s of the last)
    float comboTimer = 0.f;

    // ---- state
    Status status = Status::Playing;
    Difficulty difficulty = Difficulty::Normal;
    float gameTime = 0.f, missionTime = 0.f, waveTimer = 0.f, airDropTimer = 0.f, routeTimer = 0.f;
    float fireCd = 0.f, damageFlash = 0.f, bannerTimer = 0.f;
    int waveNumber = 0;
    std::string banner;

    // ---- pathfinding selection + statistics (for your evaluation chapter)
    bool useDijkstra = true;
    PathStat bfsStat, dijStat;
    float threatDistance = -1.f;
    ZombieKind threatKind = ZombieKind::Walker;

    std::mt19937 rng;

    static constexpr float BARRICADE_HP = 8.f;

    // ---- flow
    void newGame(Difficulty d);          // starts the campaign at mission 1
    void nextMission();                  // after Status::MissionClear
    void update(float dt, const Input& in);

    // ---- player actions
    bool toggleBarricade(Vec2 worldPos); // dynamic navigation: edits the graph
    bool undoBarricade();
    bool useMedkit();
    bool throwGrenade(Vec2 target);

    // ---- helpers used by the renderer / app
    void invalidatePaths();
    void showBanner(const std::string& s, float seconds = 2.5f) { banner = s; bannerTimer = seconds; }
    int comboMultiplier() const { return std::min(5, 1 + combo / 3); }
    float barricadeFraction(Vec2i t) const;          // 1 = new, 0 = about to break
    float waveInterval() const;
    bool routeTarget(Vec2i& out) const;              // where the yellow route points to (false = nowhere)
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

    std::vector<float> barricadeHp_;     // durability of every barricade tile (0 = none)
};

#endif
