#include "GameSave.h"
#include <cstdio>

namespace {

const int SAVE_VERSION = 3;

bool expectKey(std::FILE* f, const char* key) {
    char buf[32];
    return std::fscanf(f, "%31s", buf) == 1 && std::string(buf) == key;
}

bool readCount(std::FILE* f, const char* key, int maxCount, int& n) {
    return expectKey(f, key) && std::fscanf(f, "%d", &n) == 1 && n >= 0 && n <= maxCount;
}

}  // namespace

bool saveFileExists(const char* path) {
    std::FILE* f = std::fopen(path, "r");
    if (!f) return false;
    std::fclose(f);
    return true;
}

bool saveGameToFile(const SaveData& d, const char* path) {
    std::FILE* f = std::fopen(path, "w");
    if (!f) return false;
    std::fprintf(f, "EMS_SAVE %d\n", SAVE_VERSION);
    std::fprintf(f, "time %.4f %.4f\nwaveTimer %.4f\nairDrop %.4f\nwaveNumber %d\n", d.gameTime, d.missionTime, d.waveTimer,
                 d.airDropTimer, d.waveNumber);
    std::fprintf(f, "mission %d %d %.4f\n", d.missionIndex, d.fuelCollected, d.surviveLeft);
    std::fprintf(f, "difficulty %d\nweapon %d\nflashlight %d\n", (int)d.difficulty, d.weapon, d.flashlight);
    std::fprintf(f, "player %.3f %.3f %.3f %.3f %d %d %d %d %d %d\n", d.playerPos.x, d.playerPos.y, d.playerHp, d.stamina,
                 d.score, d.kills, d.missionKills, d.pickupsCollected, d.barricadesPlaced, d.objectivesDone);

    std::fprintf(f, "inventory %d\n", (int)d.items.size());
    for (const ItemData& it : d.items) std::fprintf(f, "%s %d\n", it.name.c_str(), it.count);

    std::fprintf(f, "waves %d\n", (int)d.waves.size());
    for (int w : d.waves) std::fprintf(f, "%d\n", w);

    std::fprintf(f, "barricades %d\n", (int)d.barricades.size());
    for (Vec2i t : d.barricades) std::fprintf(f, "%d %d\n", t.x, t.y);

    std::fprintf(f, "rubble %d\n", (int)d.rubble.size());
    for (Vec2i t : d.rubble) std::fprintf(f, "%d %d\n", t.x, t.y);

    std::fprintf(f, "zombies %d\n", (int)d.zombies.size());
    for (const ZombieData& z : d.zombies) std::fprintf(f, "%d %.3f %.3f %.3f\n", (int)z.kind, z.pos.x, z.pos.y, z.hp);

    std::fprintf(f, "pickups %d\n", (int)d.pickups.size());
    for (const PickupData& p : d.pickups) std::fprintf(f, "%d %.3f %.3f\n", (int)p.kind, p.pos.x, p.pos.y);

    bool ok = std::ferror(f) == 0;
    std::fclose(f);
    return ok;
}

bool loadGameFromFile(SaveData& out, const char* path) {
    std::FILE* f = std::fopen(path, "r");
    if (!f) return false;

    SaveData d;                      
    bool ok = true;
    int version = 0, n = 0, diff = 0;
    ok = ok && expectKey(f, "EMS_SAVE") && std::fscanf(f, "%d", &version) == 1 && version == SAVE_VERSION;
    ok = ok && expectKey(f, "time") && std::fscanf(f, "%f %f", &d.gameTime, &d.missionTime) == 2;
    ok = ok && expectKey(f, "waveTimer") && std::fscanf(f, "%f", &d.waveTimer) == 1;
    ok = ok && expectKey(f, "airDrop") && std::fscanf(f, "%f", &d.airDropTimer) == 1;
    ok = ok && expectKey(f, "waveNumber") && std::fscanf(f, "%d", &d.waveNumber) == 1;
    ok = ok && expectKey(f, "mission") && std::fscanf(f, "%d %d %f", &d.missionIndex, &d.fuelCollected, &d.surviveLeft) == 3 &&
         d.missionIndex >= 0 && d.missionIndex <= 2 && d.fuelCollected >= 0 && d.fuelCollected <= 10;
    ok = ok && expectKey(f, "difficulty") && std::fscanf(f, "%d", &diff) == 1 && diff >= 0 && diff <= 2;
    ok = ok && expectKey(f, "weapon") && std::fscanf(f, "%d", &d.weapon) == 1 && (d.weapon == 0 || d.weapon == 1);
    ok = ok && expectKey(f, "flashlight") && std::fscanf(f, "%d", &d.flashlight) == 1;
    ok = ok && expectKey(f, "player") &&
         std::fscanf(f, "%f %f %f %f %d %d %d %d %d %d", &d.playerPos.x, &d.playerPos.y, &d.playerHp, &d.stamina, &d.score,
                     &d.kills, &d.missionKills, &d.pickupsCollected, &d.barricadesPlaced, &d.objectivesDone) == 10;
    d.difficulty = (Difficulty)diff;

    if (ok && readCount(f, "inventory", 100, n)) {
        for (int i = 0; ok && i < n; ++i) {
            char name[32];
            int c = 0;
            ok = std::fscanf(f, "%31s %d", name, &c) == 2;
            if (ok) d.items.push_back({name, c});
        }
    } else ok = false;

    if (ok && readCount(f, "waves", 64, n)) {
        for (int i = 0; ok && i < n; ++i) { int w = 0; ok = std::fscanf(f, "%d", &w) == 1; if (ok) d.waves.push_back(w); }
    } else ok = false;

    if (ok && readCount(f, "barricades", COLS * ROWS, n)) {
        for (int i = 0; ok && i < n; ++i) { Vec2i t; ok = std::fscanf(f, "%d %d", &t.x, &t.y) == 2; if (ok) d.barricades.push_back(t); }
    } else ok = false;

    if (ok && readCount(f, "rubble", COLS * ROWS, n)) {
        for (int i = 0; ok && i < n; ++i) { Vec2i t; ok = std::fscanf(f, "%d %d", &t.x, &t.y) == 2; if (ok) d.rubble.push_back(t); }
    } else ok = false;

    if (ok && readCount(f, "zombies", 5000, n)) {
        for (int i = 0; ok && i < n; ++i) {
            ZombieData z;
            int kind = 0;
            ok = std::fscanf(f, "%d %f %f %f", &kind, &z.pos.x, &z.pos.y, &z.hp) == 4 && kind >= 0 && kind <= 4;
            if (ok) { z.kind = (ZombieKind)kind; d.zombies.push_back(z); }
        }
    } else ok = false;

    if (ok && readCount(f, "pickups", 5000, n)) {
        for (int i = 0; ok && i < n; ++i) {
            PickupData p;
            int kind = 0;
            ok = std::fscanf(f, "%d %f %f", &kind, &p.pos.x, &p.pos.y) == 3 && kind >= 0 && kind <= 3;
            if (ok) { p.kind = (PickupKind)kind; d.pickups.push_back(p); }
        }
    } else ok = false;

    std::fclose(f);
    if (!ok) return false;
    out = d;                         
    return true;
}
