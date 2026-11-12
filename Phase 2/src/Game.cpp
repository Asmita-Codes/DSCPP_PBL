#include "Game.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>

namespace {
const Vec2i kDirs4[4] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
int tileIndex(Vec2i t) { return t.y * COLS + t.x; }
}

Game::Game() : graph(COLS, ROWS), rng(std::random_device{}()), barricadeHp_((size_t)COLS * ROWS, 0.f) {
    graph.buildCity();
}


Vec2i Game::randomRoadTile(float minDistFromPlayer) {
    std::uniform_int_distribution<int> dc(0, COLS - 1), dr(0, ROWS - 1);
    for (int i = 0; i < 500; ++i) {
        Vec2i t{dc(rng), dr(rng)};
        if (graph.tile(t) != TileType::Road) continue;
        Vec2 c = tileCenter(t);
        if ((c - playerPos).length() < minDistFromPlayer || (c - mission.goalPos()).length() < 90.f) continue;
        return t;
    }
    return Vec2i{13, 1};
}

std::vector<Vec2i> Game::findPath(Vec2i a, Vec2i b) {
    auto t0 = std::chrono::steady_clock::now();
    std::vector<Vec2i> p = useDijkstra ? graph.dijkstra(a, b) : graph.bfs(a, b);
    double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
    (useDijkstra ? dijStat : bfsStat).add(us);
    return p;
}

void Game::invalidatePaths() {
    routeTimer = 0.f;
    for (Zombie& z : zombies) z.repath = 0.f;
}

bool Game::tileOccupied(Vec2i t) const {
    if (toTile(playerPos) == t) return true;
    for (const Zombie& z : zombies)
        if (toTile(z.pos) == t) return true;
    return false;
}

float Game::waveInterval() const { return mission.def().type == MissionType::SurviveThenReach ? 12.f : 18.f; }

int Game::zombieCount(ZombieKind k) const {
    int n = 0;
    for (const Zombie& z : zombies) if (z.kind == k) ++n;
    return n;
}

float Game::barricadeFraction(Vec2i t) const {
    if (!graph.inBounds(t)) return 0.f;
    return std::clamp(barricadeHp_[(size_t)tileIndex(t)] / BARRICADE_HP, 0.f, 1.f);
}

bool Game::routeTarget(Vec2i& out) const {
    if (mission.goalUnlocked()) { out = mission.goalTile(); return true; }
    if (mission.def().type == MissionType::CollectFuel) {      // navigation assistant: nearest fuel can
        float best = 1e9f;
        bool found = false;
        for (const Pickup& p : pickups) {
            if (p.kind != PickupKind::Fuel || !p.alive) continue;
            float d = (p.pos - playerPos).length();
            if (d < best) { best = d; out = toTile(p.pos); found = true; }
        }
        return found;
    }
    return false;
}


void Game::newGame(Difficulty d) {
    difficulty = d;
    score = 0;
    kills = 0;
    gameTime = 0.f;
    hp = 100.f;
    startMission(0);
}

void Game::nextMission() {
    if (status != Status::MissionClear) return;
    hp = std::min(100.f, hp + 35.f);               
    startMission(mission.index() + 1);
}

void Game::startMission(int idx) {
    mission.start(idx);
    graph.buildCity();
    std::fill(barricadeHp_.begin(), barricadeHp_.end(), 0.f);
    undo.clear();
    waves.clear();
    zombies.clear(); pickups.clear(); bullets.clear(); acids.clear(); grenades.clear(); explosions.clear();
    route.clear();
    threats.clear();

    playerPos = tileCenter(Vec2i{1, 1});
    stamina = 100.f; exhausted = false;
    if (idx == 0) {
        inventory.resetDefaults();
    } else {                                       
        inventory.add("Rifle", 30); inventory.add("Pistol", 15); inventory.add("Medkit", 1);
        inventory.add("Grenade", 1); inventory.add("Barrier", 2);
    }
    missionTime = 0.f; waveNumber = 0; waveTimer = 4.f; airDropTimer = 45.f;
    fireCd = 0.f; damageFlash = 0.f; routeTimer = 0.f; combo = 0; comboTimer = 0.f;
    missionKills = pickupsCollected = barricadesPlaced = 0;
    objectivesDone = 0;
    weapon = 0; flashlight = true; aimDir = Vec2(1.f, 0.f);
    bfsStat = PathStat(); dijStat = PathStat();
    threatDistance = -1.f;

    const MissionDef& d = mission.def();
    for (int i = 0; i < 6 + idx; ++i) waves.enqueue(std::max(1, (int)std::lround((4 + 2 * i) * d.waveScale)));

    
    for (int i = 0; i < 30; ++i) {
        Vec2i t = randomRoadTile(150.f);
        graph.setTile(t, TileType::Rubble);
        Vec2i r{t.x + 1, t.y};
        if (urand() < 0.6f && graph.tile(r) == TileType::Road) graph.setTile(r, TileType::Rubble);
    }
    for (int i = 0; i < 5; ++i) pickups.push_back({PickupKind::Health, tileCenter(randomRoadTile(200.f)), true});
    for (int i = 0; i < 8; ++i) pickups.push_back({PickupKind::Ammo, tileCenter(randomRoadTile(200.f)), true});
    for (int i = 0; i < 4; ++i) pickups.push_back({PickupKind::Grenade, tileCenter(randomRoadTile(200.f)), true});
    if (d.type == MissionType::CollectFuel)
        for (int i = 0; i < d.fuelNeeded; ++i) pickups.push_back({PickupKind::Fuel, tileCenter(randomRoadTile(650.f)), true});

    status = Status::Playing;
    showBanner("MISSION " + std::to_string(idx + 1) + ": " + d.title, 3.f);
}

void Game::completeMission() {
    score += 500 + std::max(0, 300 - (int)missionTime) + 100 * mission.index();
    status = mission.isLast() ? Status::Won : Status::MissionClear;
}


void Game::makeZombie(ZombieKind kind, Vec2 pos, float jitter) {
    Zombie z;
    z.pos = pos;
    z.kind = kind;
    z.jitter = jitter;
    switch (kind) {
    case ZombieKind::Walker:  z.radius = 12.f; z.maxHp = 3.f;  z.speed = 70.f;  z.damage = 15.f; z.points = 100;  z.threat = 1.0f; break;
    case ZombieKind::Runner:  z.radius = 10.f; z.maxHp = 2.f;  z.speed = 115.f; z.damage = 10.f; z.points = 150;  z.threat = 1.5f; break;
    case ZombieKind::Tank:    z.radius = 16.f; z.maxHp = 8.f;  z.speed = 48.f;  z.damage = 30.f; z.points = 300;  z.threat = 2.0f; break;
    case ZombieKind::Spitter: z.radius = 11.f; z.maxHp = 3.f;  z.speed = 60.f;  z.damage = 8.f;  z.points = 200;  z.threat = 1.8f; break;
    case ZombieKind::Boss:    z.radius = 24.f; z.maxHp = 30.f; z.speed = 52.f;  z.damage = 45.f; z.points = 1500; z.threat = 3.0f; break;
    }
    z.speed *= diffSpeed();
    z.damage *= diffDamage();
    z.hp = z.maxHp;
    zombies.push_back(z);
}

ZombieKind Game::rollKind() {
    float r = urand();
    float tank = waveNumber >= 3 ? 0.10f : 0.f;
    float spit = waveNumber >= (mission.index() == 0 ? 4 : 2) ? 0.12f : 0.f;
    float run = 0.18f + 0.03f * (float)waveNumber;
    if (r < tank) return ZombieKind::Tank;
    if (r < tank + spit) return ZombieKind::Spitter;
    if (r < tank + spit + run) return ZombieKind::Runner;
    return ZombieKind::Walker;
}

void Game::spawnWave(int baseCount) {
    ++waveNumber;
    int n = std::max(1, (int)std::lround(baseCount * diffWave()));
    for (int i = 0; i < n; ++i) makeZombie(rollKind(), tileCenter(randomRoadTile(450.f)), (float)(i % 5) * 0.05f);
    if (mission.def().bossWave == waveNumber) {
        makeZombie(ZombieKind::Boss, tileCenter(randomRoadTile(500.f)), 0.f);
        showBanner("WAVE " + std::to_string(waveNumber) + " - BOSS INCOMING!", 3.f);
    } else {
        showBanner("WAVE " + std::to_string(waveNumber) + "  (" + std::to_string(n) + " zombies)");
    }
}

void Game::registerKill(int points) {
    combo = comboTimer > 0.f ? combo + 1 : 1;
    comboTimer = 3.f;
    score += points * comboMultiplier();
    ++kills;
    ++missionKills;
}

void Game::damageZombie(Zombie& z, float dmg) {
    if (!z.alive) return;
    z.hp -= dmg;
    if (z.hp > 0.f) return;
    z.alive = false;
    registerKill(z.points);
    if (z.kind == ZombieKind::Boss) {
        pickups.push_back({PickupKind::Health, z.pos + Vec2(-18.f, 0.f), true});
        pickups.push_back({PickupKind::Ammo, z.pos + Vec2(18.f, 0.f), true});
    } else if (urand() < 0.30f) {
        int r = (int)(rng() % 3);
        pickups.push_back({r == 0 ? PickupKind::Health : r == 1 ? PickupKind::Ammo : PickupKind::Grenade, z.pos, true});
    }
}


void Game::makeNoise(Vec2 at, float radius) {
    for (Zombie& z : zombies)
        if ((z.pos - at).length() < radius) { z.alert = 5.f; z.repath = 0.f; }
}

void Game::explode(Vec2 at) {
    explosions.push_back({at, 0.f});
    for (Zombie& z : zombies)
        if ((z.pos - at).length() < 100.f) damageZombie(z, 6.f);
    if ((playerPos - at).length() < 70.f) { hp -= 20.f; damageFlash = 1.f; }   
    makeNoise(at, 750.f);
}


void Game::damageBarricade(Vec2i t, float dmg) {
    if (!graph.inBounds(t) || graph.tile(t) != TileType::Barricade) return;
    float& hpRef = barricadeHp_[(size_t)tileIndex(t)];
    hpRef -= dmg;
    if (hpRef > 0.f) return;
    hpRef = 0.f;
    graph.setTile(t, TileType::Road);                 
    invalidatePaths();
    showBanner("BARRICADE DESTROYED", 1.5f);
}


void Game::planBarricadeAttack(Zombie& z) {
    Vec2i best{-1, -1};
    float bestD = 600.f;
    for (Vec2i b : graph.tilesOfType(TileType::Barricade)) {
        float d = (tileCenter(b) - z.pos).length();
        if (d < bestD) { bestD = d; best = b; }
    }
    if (best.x < 0) return;
    std::vector<Vec2i> bestPath;
    for (Vec2i dir : kDirs4) {
        Vec2i n = best + dir;
        if (!graph.walkable(n)) continue;
        std::vector<Vec2i> p = graph.bfs(toTile(z.pos), n);
        if (!p.empty() && (bestPath.empty() || p.size() < bestPath.size())) bestPath = p;
    }
    if (bestPath.empty()) return;
    z.path = bestPath;
    z.attackTile = best;
}

bool Game::toggleBarricade(Vec2 worldPos) {
    Vec2i t = toTile(worldPos);
    if (!graph.inBounds(t) || status != Status::Playing) return false;
    if (graph.tile(t) == TileType::Barricade) {
        graph.setTile(t, TileType::Road);
        barricadeHp_[(size_t)tileIndex(t)] = 0.f;
        inventory.add("Barrier", 1);
        undo.push({ActionType::BarricadeRemoved, t});
    } else if (graph.tile(t) == TileType::Road && !tileOccupied(t) && inventory.count("Barrier") > 0) {
        graph.setTile(t, TileType::Barricade);
        barricadeHp_[(size_t)tileIndex(t)] = BARRICADE_HP;
        inventory.consume("Barrier");
        undo.push({ActionType::BarricadePlaced, t});
        ++barricadesPlaced;
        checkObjectives();
    } else return false;
    invalidatePaths();
    return true;
}

bool Game::undoBarricade() {
    Action a;
    if (!undo.pop(a)) return false;
    if (a.type == ActionType::BarricadePlaced) {
        if (graph.tile(a.tile) == TileType::Barricade) {     
            graph.setTile(a.tile, TileType::Road);
            barricadeHp_[(size_t)tileIndex(a.tile)] = 0.f;
            inventory.add("Barrier", 1);
        }
    } else if (graph.tile(a.tile) == TileType::Road && !tileOccupied(a.tile) && inventory.count("Barrier") > 0) {
        graph.setTile(a.tile, TileType::Barricade);
        barricadeHp_[(size_t)tileIndex(a.tile)] = BARRICADE_HP;
        inventory.consume("Barrier");
    }
    invalidatePaths();
    return true;
}


bool Game::useMedkit() {
    if (hp >= 100.f || inventory.count("Medkit") <= 0) return false;
    inventory.consume("Medkit");
    hp = std::min(100.f, hp + 35.f);
    return true;
}

bool Game::throwGrenade(Vec2 target) {
    if (status != Status::Playing || !inventory.consume("Grenade")) return false;
    Vec2 d = target - playerPos;
    float l = d.length();
    if (l > 350.f) target = playerPos + d / l * 350.f;     
    grenades.push_back({playerPos, target, 1.0f, true});
    return true;
}

void Game::checkObjectives() {
    bool done[3] = {missionKills >= 10, pickupsCollected >= 3, barricadesPlaced >= 2};
    for (int i = 0; i < 3; ++i)
        if (done[i] && !(objectivesDone & (1u << i))) {
            objectivesDone |= (1u << i);
            score += 200;
            showBanner(std::string("OBJECTIVE COMPLETE: ") + objectiveText(i), 2.5f);
        }
}


void Game::airDrop() {
    std::uniform_int_distribution<int> dc(0, COLS - 1), dr(0, ROWS - 1);
    for (int i = 0; i < 400; ++i) {
        Vec2i t{dc(rng), dr(rng)};
        if (graph.tile(t) != TileType::Road) continue;
        float d = (tileCenter(t) - playerPos).length();
        if (d < 120.f || d > 320.f) continue;
        int r = (int)(rng() % 3);
        pickups.push_back({r == 0 ? PickupKind::Health : r == 1 ? PickupKind::Ammo : PickupKind::Grenade, tileCenter(t), true});
        showBanner("AIR DROP NEARBY!", 2.f);
        return;
    }
}


void Game::moveAlongPath(Zombie& z, float speed, float dt) {
    Vec2 target = playerPos;
    bool go = true;
    if (z.attackTile.x >= 0) {                      
        if (z.path.size() >= 2) target = tileCenter(z.path[1]);
        else go = false;
    } else if (z.path.size() >= 2) target = tileCenter(z.path[1]);
    else if (z.path.empty()) go = false;           
    if (!go) return;
    Vec2 v = target - z.pos;
    float vl = v.length();
    if (vl > 1.f) z.pos += v / vl * (speed * dt);
}

void Game::updateZombie(Zombie& z, float dt) {
    float d = (playerPos - z.pos).length();
    z.alert = std::max(0.f, z.alert - dt);
    z.shootTimer -= dt;
    float speed = z.speed;
    if (graph.tile(toTile(z.pos)) == TileType::Rubble) speed *= 0.5f;

    if (z.kind == ZombieKind::Boss) {                
        z.abilityTimer -= dt;
        if (z.charging > 0.f) { z.charging -= dt; speed *= 2.6f; }
        else if (z.abilityTimer <= 0.f && d < 450.f) { z.charging = 1.2f; z.abilityTimer = 6.f; }
    }

    bool active = d < ZOMBIE_AGGRO || z.alert > 0.f;
    if (active) {
        z.repath -= dt;
        if (z.repath <= 0.f) {
            z.path = findPath(toTile(z.pos), toTile(playerPos));
            z.repath = 0.5f + z.jitter;
            z.attackTile = Vec2i{-1, -1};
            if (z.path.empty()) planBarricadeAttack(z);
        }
        bool spitter = z.kind == ZombieKind::Spitter;
        if (spitter && d < 150.f) {                 
            Vec2 away = z.pos - playerPos;
            float l = away.length();
            if (l > 0.5f) {
                Vec2 np = z.pos + away / l * (speed * dt);
                if (graph.canStand(np, z.radius)) z.pos = np;
            }
        } else if (!(spitter && d < 300.f)) {        
            moveAlongPath(z, speed, dt);
        }
        if (spitter && d < 330.f && z.shootTimer <= 0.f) {
            Vec2 dir = (playerPos - z.pos) / std::max(d, 1.f);
            acids.push_back({z.pos, dir * 230.f, 2.0f, true});
            z.shootTimer = 2.2f;
        }
    }

    if (z.attackTile.x >= 0) {                       
        if (graph.tile(z.attackTile) != TileType::Barricade) {
            z.attackTile = Vec2i{-1, -1};
        } else {
            Vec2i zt = toTile(z.pos);
            if (std::abs(zt.x - z.attackTile.x) + std::abs(zt.y - z.attackTile.y) <= 1) {
                float dps = z.kind == ZombieKind::Boss ? 8.f : z.kind == ZombieKind::Tank ? 4.f : 2.f;
                damageBarricade(z.attackTile, dps * dt);
            }
        }
    }

    if (d < z.radius + PLAYER_RADIUS + 2.f) hp -= z.damage * dt;   
}


void Game::update(float dt, const Input& in) {
    if (status != Status::Playing) return;
    gameTime += dt;
    missionTime += dt;
    aimDir = in.aim;
    float hpBefore = hp;

    bool wasUnlocked = mission.goalUnlocked();
    mission.update(dt);
    if (!wasUnlocked && mission.goalUnlocked()) showBanner("GOAL UNLOCKED - GO GO GO!", 3.f);

    // --- player movement: sprint uses stamina, rubble halves the speed
    Vec2 dir = in.move;
    float l = dir.length();
    bool moving = l > 0.1f;
    if (l > 0.f) dir = dir / l;
    bool sprinting = in.sprint && moving && stamina > 0.f && !exhausted;
    if (sprinting) {
        stamina -= 35.f * dt;
        if (stamina <= 0.f) { stamina = 0.f; exhausted = true; }
    } else {
        stamina = std::min(100.f, stamina + 20.f * dt);
        if (exhausted && stamina >= 25.f) exhausted = false;
    }
    float speed = 150.f * (sprinting ? 1.6f : 1.f);
    if (graph.tile(toTile(playerPos)) == TileType::Rubble) speed *= 0.5f;
    Vec2 nx(playerPos.x + dir.x * speed * dt, playerPos.y);
    if (graph.canStand(nx, PLAYER_RADIUS)) playerPos = nx;
    Vec2 ny(playerPos.x, playerPos.y + dir.y * speed * dt);
    if (graph.canStand(ny, PLAYER_RADIUS)) playerPos = ny;


    fireCd -= dt;
    if (in.fire && fireCd <= 0.f) {
        if (inventory.consume(weapon == 0 ? "Rifle" : "Pistol")) {
            float ang = std::atan2(in.aim.y, in.aim.x) + (weapon == 0 ? (urand() - 0.5f) * 0.06f : 0.f);
            Vec2 d(std::cos(ang), std::sin(ang));
            bullets.push_back({playerPos + d * 16.f, d * (weapon == 0 ? 720.f : 620.f), 1.0f, weapon == 0 ? 1 : 2, true});
            fireCd = weapon == 0 ? 0.12f : 0.35f;
            makeNoise(playerPos, weapon == 0 ? 650.f : 380.f);
        }
    }

    waveTimer -= dt;
    if (!waves.empty() && waveTimer <= 0.f) {
        int n = 0;
        waves.dequeue(n);
        spawnWave(n);
        waveTimer = waveInterval();
    }


    airDropTimer -= dt;
    if (airDropTimer <= 0.f) { airDrop(); airDropTimer = 45.f; }


    routeTimer -= dt;
    if (routeTimer <= 0.f) {
        Vec2i target;
        route = routeTarget(target) ? findPath(toTile(playerPos), target) : std::vector<Vec2i>();
        routeTimer = 0.25f;
    }

 
    for (size_t i = 0; i < zombies.size(); ++i) updateZombie(zombies[i], dt);

    for (Acid& a : acids) {
        a.pos += a.vel * dt;
        a.life -= dt;
        if (a.life <= 0.f || graph.isBuilding(toTile(a.pos))) { a.alive = false; continue; }
        if ((a.pos - playerPos).length() < PLAYER_RADIUS + 4.f) {
            hp -= 12.f * diffDamage();
            damageFlash = std::min(1.f, damageFlash + 0.4f);
            a.alive = false;
        }
    }


    for (Bullet& b : bullets) {
        b.pos += b.vel * dt;
        b.life -= dt;
        if (b.life <= 0.f || graph.isBuilding(toTile(b.pos))) { b.alive = false; continue; }
        for (Zombie& z : zombies) {
            if (!z.alive || (z.pos - b.pos).length() > z.radius + 4.f) continue;
            damageZombie(z, (float)b.damage);
            b.alive = false;
            break;
        }
    }

    for (Grenade& g : grenades) {
        Vec2 d = g.target - g.pos;
        float dl = d.length();
        if (dl > 4.f) g.pos += d / dl * std::min(dl, 420.f * dt);
        g.fuse -= dt;
        if (g.fuse <= 0.f) { g.alive = false; explode(g.pos); }
    }
    for (Explosion& e : explosions) e.t += dt;


    for (Pickup& k : pickups) {
        if (!k.alive || (k.pos - playerPos).length() > 9.f + PLAYER_RADIUS) continue;
        k.alive = false;
        if (k.kind == PickupKind::Health) inventory.add("Medkit", 1);
        else if (k.kind == PickupKind::Ammo) { inventory.add("Rifle", 20); inventory.add("Pistol", 10); }
        else if (k.kind == PickupKind::Grenade) inventory.add("Grenade", 1);
        else {
            mission.addFuel();
            showBanner("FUEL " + std::to_string(mission.fuel()) + "/" + std::to_string(mission.def().fuelNeeded), 2.f);
        }
        score += 25;
        ++pickupsCollected;
    }
    checkObjectives();

    if (comboTimer > 0.f) { comboTimer -= dt; if (comboTimer <= 0.f) combo = 0; }

  
    threats.clear();
    for (size_t i = 0; i < zombies.size(); ++i)
        if (zombies[i].alive) threats.push((zombies[i].pos - playerPos).length() / zombies[i].threat, (int)i);
    PQItem top;
    if (threats.peek(top)) {
        threatDistance = (zombies[(size_t)top.id].pos - playerPos).length();
        threatKind = zombies[(size_t)top.id].kind;
    } else threatDistance = -1.f;

 
    zombies.erase(std::remove_if(zombies.begin(), zombies.end(), [](const Zombie& z) { return !z.alive; }), zombies.end());
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) { return !b.alive; }), bullets.end());
    acids.erase(std::remove_if(acids.begin(), acids.end(), [](const Acid& a) { return !a.alive; }), acids.end());
    pickups.erase(std::remove_if(pickups.begin(), pickups.end(), [](const Pickup& p) { return !p.alive; }), pickups.end());
    grenades.erase(std::remove_if(grenades.begin(), grenades.end(), [](const Grenade& g) { return !g.alive; }), grenades.end());
    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& e) { return e.t > 0.45f; }), explosions.end());


    if (hp < hpBefore) damageFlash = std::min(1.f, damageFlash + 0.05f + (hpBefore - hp) * 0.03f);
    damageFlash = std::max(0.f, damageFlash - 1.2f * dt);
    bannerTimer = std::max(0.f, bannerTimer - dt);

    if (hp <= 0.f) {
        hp = 0.f;
        status = Status::Lost;
    } else if (mission.goalUnlocked() && (playerPos - mission.goalPos()).length() < 40.f) {
        completeMission();
    }
}


SaveData Game::toSave() const {
    SaveData d;
    d.gameTime = gameTime; d.missionTime = missionTime; d.waveTimer = waveTimer; d.airDropTimer = airDropTimer;
    d.waveNumber = waveNumber; d.difficulty = difficulty;
    d.missionIndex = mission.index(); d.fuelCollected = mission.fuel(); d.surviveLeft = mission.surviveLeft();
    d.weapon = weapon; d.flashlight = flashlight ? 1 : 0;
    d.playerPos = playerPos; d.playerHp = hp; d.stamina = stamina;
    d.score = score; d.kills = kills; d.missionKills = missionKills;
    d.pickupsCollected = pickupsCollected; d.barricadesPlaced = barricadesPlaced; d.objectivesDone = (int)objectivesDone;
    for (const ItemNode* n = inventory.head(); n; n = n->next) d.items.push_back({n->name, n->count});
    for (int i = 0; i < waves.size(); ++i) { int v = 0; waves.at(i, v); d.waves.push_back(v); }
    d.barricades = graph.tilesOfType(TileType::Barricade);
    d.rubble = graph.tilesOfType(TileType::Rubble);
    for (const Zombie& z : zombies) d.zombies.push_back({z.pos, z.hp, z.kind});
    for (const Pickup& p : pickups) d.pickups.push_back({p.kind, p.pos});
    return d;
}

void Game::fromSave(const SaveData& d) {
    difficulty = d.difficulty;
    mission.restore(d.missionIndex, d.fuelCollected, d.surviveLeft);
    graph.buildCity();
    std::fill(barricadeHp_.begin(), barricadeHp_.end(), 0.f);
    for (Vec2i t : d.rubble) if (graph.tile(t) == TileType::Road) graph.setTile(t, TileType::Rubble);
    for (Vec2i t : d.barricades)
        if (graph.tile(t) == TileType::Road) { graph.setTile(t, TileType::Barricade); barricadeHp_[(size_t)tileIndex(t)] = BARRICADE_HP; }
    inventory.clear();
    for (const ItemData& it : d.items) inventory.set(it.name.c_str(), it.count);
    waves.clear();
    for (int w : d.waves) waves.enqueue(w);
    zombies.clear(); pickups.clear(); bullets.clear(); acids.clear(); grenades.clear(); explosions.clear();
    route.clear(); undo.clear(); threats.clear();
    playerPos = graph.canStand(d.playerPos, PLAYER_RADIUS) ? d.playerPos : tileCenter(Vec2i{1, 1});
    hp = std::clamp(d.playerHp, 1.f, 100.f);
    stamina = std::clamp(d.stamina, 0.f, 100.f);
    exhausted = false;
    score = d.score; kills = d.kills; missionKills = d.missionKills;
    pickupsCollected = d.pickupsCollected; barricadesPlaced = d.barricadesPlaced;
    objectivesDone = (unsigned)d.objectivesDone & 7u;
    weapon = d.weapon; flashlight = d.flashlight != 0;
    gameTime = d.gameTime; missionTime = d.missionTime; waveTimer = d.waveTimer; airDropTimer = d.airDropTimer;
    waveNumber = d.waveNumber;
    combo = 0; comboTimer = 0.f;
    for (const ZombieData& z : d.zombies) {
        makeZombie(z.kind, z.pos, 0.f);
        zombies.back().hp = std::clamp(z.hp, 0.5f, zombies.back().maxHp);
    }
    for (const PickupData& p : d.pickups) pickups.push_back({p.kind, p.pos, true});
    status = Status::Playing;
    damageFlash = 0.f; fireCd = 0.f; routeTimer = 0.f;
    showBanner("GAME LOADED", 1.5f);
}
