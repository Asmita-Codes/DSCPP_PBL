#ifndef GAMETYPES_H
#define GAMETYPES_H

#include <cmath>
#include <string>
#include <vector>


struct Vec2 {
    float x = 0.f, y = 0.f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(Vec2 o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(Vec2 o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float k) const { return Vec2(x * k, y * k); }
    Vec2 operator/(float k) const { return Vec2(x / k, y / k); }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    float length() const { return std::hypot(x, y); }
};

struct Vec2i {
    int x = 0, y = 0;
    bool operator==(Vec2i o) const { return x == o.x && y == o.y; }
    bool operator!=(Vec2i o) const { return !(*this == o); }
    Vec2i operator+(Vec2i o) const { return Vec2i{x + o.x, y + o.y}; }
};


constexpr int COLS = 60, ROWS = 40, TILE = 32;
constexpr float WORLD_W = (float)(COLS * TILE), WORLD_H = (float)(ROWS * TILE);
constexpr float PI = 3.14159265f;
constexpr float PLAYER_RADIUS = 11.f;
constexpr float ZOMBIE_AGGRO = 520.f;               
constexpr Vec2i SAFE_TILE{49, 31};                  
inline Vec2 tileCenter(Vec2i t) { return Vec2(t.x * TILE + TILE / 2.f, t.y * TILE + TILE / 2.f); }
inline Vec2i toTile(Vec2 p) { return Vec2i{(int)std::floor(p.x / TILE), (int)std::floor(p.y / TILE)}; }


enum class TileType : unsigned char { Road, Building, Barricade, Rubble };
enum class ZombieKind { Walker, Runner, Tank, Spitter, Boss };
enum class PickupKind { Health, Ammo, Grenade, Fuel };
enum class Difficulty { Easy, Normal, Hard };

struct ItemData { std::string name; int count; };
struct ZombieData { Vec2 pos; float hp; ZombieKind kind; };
struct PickupData { PickupKind kind; Vec2 pos; };

struct SaveData {
    float gameTime = 0.f, missionTime = 0.f, waveTimer = 0.f, airDropTimer = 0.f, surviveLeft = 0.f;
    int waveNumber = 0;
    int missionIndex = 0, fuelCollected = 0;
    Difficulty difficulty = Difficulty::Normal;
    int weapon = 0;
    int flashlight = 1;
    Vec2 playerPos;
    float playerHp = 100.f, stamina = 100.f;
    int score = 0, kills = 0, missionKills = 0, pickupsCollected = 0, barricadesPlaced = 0;
    int objectivesDone = 0;                 
    std::vector<ItemData> items;
    std::vector<int> waves;                 
    std::vector<Vec2i> barricades;
    std::vector<Vec2i> rubble;
    std::vector<ZombieData> zombies;
    std::vector<PickupData> pickups;
};

#endif
