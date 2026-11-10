#include "Fx.h"
#include <algorithm>
#include <cmath>

namespace {
float angleOf(Vec2 v) { return std::atan2(v.y, v.x); }
float wrapPi(float a) { while (a > PI) a -= 2 * PI; while (a < -PI) a += 2 * PI; return a; }
float dist(Vec2 a, Vec2 b) { return (a - b).length(); }
int kindPoints(ZombieKind k) {
    switch (k) {
    case ZombieKind::Walker: return 100;
    case ZombieKind::Runner: return 150;
    case ZombieKind::Tank: return 300;
    case ZombieKind::Spitter: return 200;
    case ZombieKind::Boss: return 1500;
    }
    return 100;
}
const char* pickupLabel(PickupKind k) {
    switch (k) {
    case PickupKind::Health: return "+MEDKIT";
    case PickupKind::Ammo: return "+AMMO";
    case PickupKind::Grenade: return "+GRENADE";
    case PickupKind::Fuel: return "+FUEL";
    }
    return "+";
}
Col pickupColor(PickupKind k) {
    switch (k) {
    case PickupKind::Health: return Col(255, 110, 110);
    case PickupKind::Ammo: return Col(255, 214, 90);
    case PickupKind::Grenade: return Col(150, 230, 120);
    case PickupKind::Fuel: return Col(255, 150, 60);
    }
    return C::White;
}
}  // namespace

void Fx::emit(PType t, Vec2 p, Vec2 v, float life, float size, Col c, float grow, float drag, float spin) {
    if (parts.size() > 2500) return;
    parts.push_back(Particle{t, p, v, life, life, size, grow, rnd(0.f, 6.28f), spin, drag, c});
}

void Fx::decal(DType t, Vec2 p, float size, float life, int var, float rot) {
    if (decals.size() >= 320) decals.erase(decals.begin());               
    decals.push_back(Decal{t, p, rot < 0.f ? rnd(0.f, 360.f) : rot, size, 0.f, life, var});
}

void Fx::bloodBurst(Vec2 p, Vec2 dir, int n, float force) {
    float base = angleOf(dir);
    for (int i = 0; i < n; ++i) {
        float a = base + rnd(-0.7f, 0.7f), s = rnd(0.3f, 1.f) * force;
        emit(PType::Blood, p, Vec2(std::cos(a), std::sin(a)) * s, rnd(0.25f, 0.6f), rnd(1.6f, 3.4f),
             Col((int)rnd(120, 190), 10, 14), 0.f, 5.f);
    }
}

void Fx::explosion(Vec2 p, const Game& w) {
    float d = dist(p, w.playerPos);
    shake = std::max(shake, 16.f * clamp01(1.2f - d / 700.f));
    for (int i = 0; i < 34; ++i) {                                            
        float a = rnd(0, 6.28f), s = rnd(60, 340);
        emit(PType::Ember, p, Vec2(std::cos(a), std::sin(a)) * s, rnd(0.25f, 0.8f), rnd(2.f, 5.f), Col(255, (int)rnd(120, 220), 60), -2.f, 3.5f);
    }
    for (int i = 0; i < 16; ++i) {                                            
        float a = rnd(0, 6.28f), s = rnd(10, 90);
        emit(PType::Smoke, p + Vec2(std::cos(a), std::sin(a)) * rnd(0, 30), Vec2(std::cos(a), std::sin(a)) * s,
             rnd(1.2f, 2.6f), rnd(22, 40), Col(70, 66, 62), 30.f, 1.6f, rnd(-1.f, 1.f));
    }
    for (int i = 0; i < 16; ++i) {                                            
        float a = rnd(0, 6.28f), s = rnd(120, 300);
        emit(PType::Chip, p, Vec2(std::cos(a), std::sin(a)) * s, rnd(0.4f, 0.9f), rnd(2.f, 4.f), Col(90, 84, 76), 0.f, 4.f, rnd(-12, 12));
    }
    decal(DType::Scorch, p, rnd(150, 180), 120.f);
    cue(Sfx::Explosion, p);
}

Vec2 Fx::shakeOffset(double t) const {
    if (shake < 0.05f) return Vec2();
    return Vec2((float)(std::sin(t * 83.0) + std::sin(t * 47.0 + 1.3)) * 0.5f * shake,
                (float)(std::cos(t * 71.0) + std::sin(t * 39.0 + 0.4)) * 0.5f * shake);
}


void Fx::snapshot(const Game& w) {
    prevPlayer_ = w.playerPos;
    prevHp_ = w.hp;
    prevP2_ = w.p2.pos;
    prevHp2_ = w.p2.hp;
    prevWeapon2_ = w.p2.weapon;
    prevKills_ = w.kills;
    prevMedkits_ = w.inventory.count("Medkit");
    prevWeapon_ = w.weapon;
    prevWave_ = w.waveNumber;
    prevObjectives_ = w.objectivesDone;
    prevFlash_ = w.flashlight;
    prevUnlocked_ = w.mission.goalUnlocked();
    prevBullets_ = w.bullets;
    prevAcids_ = w.acids;
    if (prevVersion_ != w.graph.version() || prevTiles_.empty()) {
        prevTiles_.resize((size_t)COLS * ROWS);
        for (int y = 0; y < ROWS; ++y)
            for (int x = 0; x < COLS; ++x) prevTiles_[(size_t)y * COLS + x] = w.graph.tile(Vec2i{x, y});
        prevVersion_ = w.graph.version();
    }
}

void Fx::reset(const Game& w) {
    parts.clear(); decals.clear(); popups.clear(); cues.clear();
    zv.clear(); pv.clear(); gv.clear();
    for (const Zombie& z : w.zombies) zv.push_back(ZVis{z.pos, z.kind, z.hp, angleOf(w.playerPos - z.pos), rnd(0, 6.28f), 0.f, 9.f, z.charging});
    for (const Pickup& p : w.pickups) pv.push_back(PickVis{p.pos, p.kind, 9.f});
    for (const Grenade& g : w.grenades) gv.push_back(GrenVis{g.target, std::max(1.f, dist(g.pos, g.target))});
    shake = muzzle = recoil = hurt = 0.f;
    muzzle2 = recoil2 = hurt2 = 0.f;
    prevTiles_.clear();
    prevVersion_ = ~0u;
    snapshot(w);
}

void Fx::update(const Game& w, float dt, bool observe) {
    if (observe) {
        
        std::vector<ZVis> next;
        next.reserve(w.zombies.size());
        size_t j = 0;
        float tol = 18.f + 420.f * dt;
        for (const ZVis& o : zv) {
            if (j < w.zombies.size() && w.zombies[j].kind == o.kind && dist(w.zombies[j].pos, o.pos) < tol) {
                const Zombie& z = w.zombies[j++];
                ZVis n = o;
                Vec2 d = z.pos - o.pos;
                float moved = d.length();
                float want = moved > 0.05f ? angleOf(d) : angleOf(w.playerPos - z.pos);
                n.heading += wrapPi(want - n.heading) * clamp01(dt * 8.f);
                n.phase += moved * 0.16f;
                n.flash = std::max(0.f, n.flash - dt * 5.f);
                if (z.hp < o.hp - 0.01f) {                                   
                    n.flash = 1.f;
                    Vec2 dir = z.pos - w.playerPos;
                    float l = dir.length();
                    bloodBurst(z.pos, l > 1.f ? dir / l : Vec2(1, 0), 7, 170.f);
                    cue(Sfx::ZHit, z.pos, 1.f, z.kind == ZombieKind::Tank || z.kind == ZombieKind::Boss ? 0.8f : 1.f);
                }
                if (z.charging > 0.f && o.charging <= 0.f) cue(Sfx::Roar, z.pos, 0.9f, 1.15f);   
                n.charging = z.charging;
                n.pos = z.pos; n.hp = z.hp; n.age += dt;
                next.push_back(n);
            } else {                                                        
                Vec2 dir = o.pos - w.playerPos;
                float l = dir.length();
                dir = l > 1.f ? dir / l : Vec2(1, 0);
                bool boss = o.kind == ZombieKind::Boss;
                bloodBurst(o.pos, dir, boss ? 40 : 18, boss ? 320.f : 240.f);
                float r = boss ? 24.f : o.kind == ZombieKind::Tank ? 16.f : 12.f;
                decal(DType::Blood, o.pos + dir * 6.f, r * (boss ? 6.f : 4.f), 60.f, (int)rnd(0, 2.99f));
                decal(DType::Corpse, o.pos, r, 30.f, (int)o.kind, o.heading * 180.f / PI + rnd(-30, 30));
                if (o.kind == ZombieKind::Spitter)
                    for (int i = 0; i < 12; ++i) { float a = rnd(0, 6.28f); emit(PType::Acid, o.pos, Vec2(std::cos(a), std::sin(a)) * rnd(40, 160), rnd(0.3f, 0.7f), rnd(2, 4), Col(170, 255, 80), 0.f, 4.f); }
                int pts = kindPoints(o.kind) * w.comboMultiplier();
                popups.push_back(Popup{o.pos, "+" + std::to_string(pts), boss ? C::Accent : Col(255, 235, 160), 0.f});
                if (boss) shake = std::max(shake, 18.f);
                cue(Sfx::ZDeath, o.pos, boss ? 1.f : 0.9f, boss ? 0.7f : o.kind == ZombieKind::Runner ? 1.2f : 1.f);
                if (boss) cue(Sfx::Roar, o.pos, 1.f, 0.7f);
            }
        }
        for (; j < w.zombies.size(); ++j) {                                 
            const Zombie& z = w.zombies[j];
            next.push_back(ZVis{z.pos, z.kind, z.hp, angleOf(w.playerPos - z.pos), rnd(0, 6.28f), 0.f, 0.f, z.charging});
            if (z.kind == ZombieKind::Boss) cue(Sfx::Roar, z.pos, 1.f, 0.9f);
            else cue(Sfx::Groan, z.pos, 0.7f, z.kind == ZombieKind::Tank ? 0.75f : z.kind == ZombieKind::Runner ? 1.25f : 1.f);
            for (int i = 0; i < 8; ++i) { float a = rnd(0, 6.28f); emit(PType::Dust, z.pos, Vec2(std::cos(a), std::sin(a)) * rnd(20, 60), rnd(0.6f, 1.1f), rnd(8, 14), Col(110, 100, 90), 14.f, 2.5f); }
        }
        zv.swap(next);

        
        std::vector<char> matched(w.bullets.size(), 0);
        for (const Bullet& o : prevBullets_) {
            bool found = false;
            Vec2 expect = o.pos + o.vel * dt;
            for (size_t k = 0; k < w.bullets.size(); ++k)
                if (!matched[k] && w.bullets[k].vel.x == o.vel.x && w.bullets[k].vel.y == o.vel.y && dist(w.bullets[k].pos, expect) < 2.f) {
                    matched[k] = 1; found = true; break;
                }
            if (found || o.life - dt <= 0.f) continue;
            bool hitZombie = false;
            for (const ZVis& z : zv) if (dist(z.pos, expect) < 30.f) { hitZombie = true; break; }
            if (hitZombie) continue;
            Vec2 back = o.vel * (-1.f / std::max(1.f, o.vel.length()));
            for (int i = 0; i < 7; ++i) {                                   
                float a = angleOf(back) + rnd(-1.1f, 1.1f);
                emit(PType::Spark, expect, Vec2(std::cos(a), std::sin(a)) * rnd(90, 260), rnd(0.12f, 0.3f), rnd(1.f, 2.f), Col(255, 220, 140), 0.f, 6.f);
            }
            emit(PType::Dust, expect + back * 3.f, back * 20.f, 0.6f, 6.f, Col(140, 130, 120), 16.f, 3.f);
            cue(Sfx::Ricochet, expect, 1.f, rnd(0.85f, 1.2f));
        }
        for (size_t k = 0; k < w.bullets.size(); ++k) {
            if (matched[k]) continue;                                       
            float a = angleOf(w.bullets[k].vel);
            const bool byP2 = w.coop && dist(w.bullets[k].pos, w.p2.pos) < dist(w.bullets[k].pos, w.playerPos);  
            const Vec2 shooter = byP2 ? w.p2.pos : w.playerPos;
            const int wpn = byP2 ? w.p2.weapon : w.weapon;
            if (byP2) { muzzle2 = 0.07f; recoil2 = 1.f; } else { muzzle = 0.07f; recoil = 1.f; }
            shake = std::max(shake, wpn == 0 ? 1.6f : 2.6f);
            cue(wpn == 0 ? Sfx::Rifle : Sfx::Pistol, shooter);
            Vec2 side(-std::sin(a), std::cos(a));
            emit(PType::Shell, shooter + Vec2(std::cos(a), std::sin(a)) * 8.f, side * rnd(70, 120) + Vec2(std::cos(a), std::sin(a)) * rnd(-30, 10),
                 rnd(0.5f, 0.8f), 2.2f, Col(230, 180, 70), 0.f, 6.f, rnd(-25, 25));
            for (int i = 0; i < 3; ++i)
                emit(PType::Smoke, w.bullets[k].pos, Vec2(std::cos(a), std::sin(a)) * rnd(20, 60), rnd(0.3f, 0.6f), rnd(5, 8), Col(150, 150, 150), 18.f, 3.f);
        }

        
        std::vector<char> acidSeen(w.acids.size(), 0);
        for (const Acid& o : prevAcids_) {
            Vec2 expect = o.pos + o.vel * dt;
            bool found = false;
            for (size_t k = 0; k < w.acids.size(); ++k)
                if (!acidSeen[k] && w.acids[k].vel.x == o.vel.x && w.acids[k].vel.y == o.vel.y && dist(w.acids[k].pos, expect) < 2.f) { acidSeen[k] = 1; found = true; break; }
            if (found || o.life - dt <= 0.f) continue;
            for (int i = 0; i < 10; ++i) { float a = rnd(0, 6.28f); emit(PType::Acid, expect, Vec2(std::cos(a), std::sin(a)) * rnd(30, 140), rnd(0.25f, 0.6f), rnd(1.5f, 3.f), Col(170, 255, 80), 0.f, 5.f); }
            decal(DType::Acid, expect, rnd(18, 26), 12.f);
            cue(Sfx::Splash, expect);
        }
        for (size_t k = 0; k < w.acids.size(); ++k)
            if (!acidSeen[k]) cue(Sfx::Spit, w.acids[k].pos);

        
        for (const Explosion& e : w.explosions)
            if (e.t <= dt * 1.01f + 1e-4f) explosion(e.pos, w);


        {
            std::vector<GrenVis> ng;
            size_t a = 0;
            for (const Grenade& g : w.grenades) {
                while (a < gv.size() && !(gv[a].target.x == g.target.x && gv[a].target.y == g.target.y)) ++a;
                if (a < gv.size()) ng.push_back(gv[a++]);
                else { ng.push_back(GrenVis{g.target, std::max(1.f, dist(g.pos, g.target) + 420.f * dt)}); cue(Sfx::Throw, w.playerPos); }
            }
            gv.swap(ng);
        }

        
        {
            std::vector<PickVis> np;
            size_t a = 0;
            for (const Pickup& p : w.pickups) {
                while (a < pv.size() && !(pv[a].pos.x == p.pos.x && pv[a].pos.y == p.pos.y && pv[a].kind == p.kind)) {
                    const PickVis& gone = pv[a++];
                    if (dist(gone.pos, w.playerPos) < 40.f) {
                        popups.push_back(Popup{gone.pos, pickupLabel(gone.kind), pickupColor(gone.kind), 0.f});
                        cue(gone.kind == PickupKind::Fuel ? Sfx::Fuel : Sfx::Pickup, gone.pos);
                        for (int i = 0; i < 12; ++i) { float ang = rnd(0, 6.28f); emit(PType::Glint, gone.pos, Vec2(std::cos(ang), std::sin(ang)) * rnd(40, 120), rnd(0.3f, 0.6f), rnd(1.5f, 3.f), pickupColor(gone.kind), 0.f, 3.f); }
                    }
                }
                if (a < pv.size()) { PickVis v = pv[a++]; v.age += dt; np.push_back(v); }
                else np.push_back(PickVis{p.pos, p.kind, 0.f});
            }
            for (; a < pv.size(); ++a)
                if (dist(pv[a].pos, w.playerPos) < 40.f) {
                    popups.push_back(Popup{pv[a].pos, pickupLabel(pv[a].kind), pickupColor(pv[a].kind), 0.f});
                    cue(pv[a].kind == PickupKind::Fuel ? Sfx::Fuel : Sfx::Pickup, pv[a].pos);
                    for (int i = 0; i < 12; ++i) { float ang = rnd(0, 6.28f); emit(PType::Glint, pv[a].pos, Vec2(std::cos(ang), std::sin(ang)) * rnd(40, 120), rnd(0.3f, 0.6f), rnd(1.5f, 3.f), pickupColor(pv[a].kind), 0.f, 3.f); }
                }
            pv.swap(np);
        }

        
        if (w.graph.version() != prevVersion_ && !prevTiles_.empty()) {
            for (int y = 0; y < ROWS; ++y)
                for (int x = 0; x < COLS; ++x) {
                    TileType was = prevTiles_[(size_t)y * COLS + x], now = w.graph.tile(Vec2i{x, y});
                    if (was == now) continue;
                    Vec2 c = tileCenter(Vec2i{x, y});
                    if (was == TileType::Barricade) cue(Sfx::Break, c);
                    if (now == TileType::Barricade) cue(Sfx::Place, c);
                    if (was == TileType::Barricade)
                        for (int i = 0; i < 16; ++i) { float a = rnd(0, 6.28f); emit(PType::Chip, c, Vec2(std::cos(a), std::sin(a)) * rnd(60, 220), rnd(0.4f, 0.9f), rnd(2.f, 4.f), Col(176, 122, 66), 0.f, 4.f, rnd(-14, 14)); }
                    if (now == TileType::Barricade)
                        for (int i = 0; i < 10; ++i) { float a = rnd(0, 6.28f); emit(PType::Dust, c, Vec2(std::cos(a), std::sin(a)) * rnd(30, 80), rnd(0.4f, 0.8f), rnd(6, 10), Col(130, 120, 105), 16.f, 3.f); }
                }
        }
        for (const Zombie& z : w.zombies)                                  
            if (z.attackTile.x >= 0 && rnd(0, 1) < dt * 5.f) {
                Vec2 c = tileCenter(z.attackTile);
                Vec2 d = z.pos - c;
                float l = std::max(1.f, d.length());
                cue(Sfx::Chew, c, 1.f, rnd(0.8f, 1.2f));
                emit(PType::Chip, c + d / l * 12.f, d / l * rnd(40, 110) + Vec2(rnd(-40, 40), rnd(-40, 40)), rnd(0.3f, 0.6f), rnd(1.5f, 3.f), Col(176, 122, 66), 0.f, 4.f, rnd(-14, 14));
            }

        if (w.hp < prevHp_) {
            float dmg = prevHp_ - w.hp;
            hurtAccum_ += dmg;
            hurt = std::min(1.f, hurt + dmg * 0.04f);
            shake = std::max(shake, std::min(7.f, 1.f + dmg * 0.4f));
            if (hurtAccum_ > 3.f) {
                bloodBurst(w.playerPos, Vec2(rnd(-1, 1), rnd(-1, 1)), 4, 120.f);
                cue(Sfx::Hurt, w.playerPos, 1.f, rnd(0.9f, 1.1f));
                hurtAccum_ = 0.f;
            }
        }
        float moved = dist(w.playerPos, prevPlayer_);
        if (w.coop) {                                                    
            if (w.p2.hp < prevHp2_) {
                float dmg = prevHp2_ - w.p2.hp;
                hurtAccum2_ += dmg;
                hurt2 = std::min(1.f, hurt2 + dmg * 0.04f);
                shake = std::max(shake, std::min(5.f, 1.f + dmg * 0.3f));
                if (hurtAccum2_ > 3.f) { bloodBurst(w.p2.pos, Vec2(rnd(-1, 1), rnd(-1, 1)), 4, 120.f); cue(Sfx::Hurt, w.p2.pos, 1.f, rnd(1.1f, 1.25f)); hurtAccum2_ = 0.f; }
            }
            float m2 = dist(w.p2.pos, prevP2_), b2 = walkPhase2;
            walkPhase2 += m2 * 0.12f;
            moveAmt2 += ((m2 > 0.2f ? 1.f : 0.f) - moveAmt2) * clamp01(dt * 10.f);
            if (std::floor(walkPhase2 / (2.f * PI)) != std::floor(b2 / (2.f * PI))) cue(Sfx::Footstep, w.p2.pos, 1.f, rnd(1.f, 1.15f));
            if (w.p2.weapon != prevWeapon2_) cue(Sfx::Switch, w.p2.pos);
        }
        float before = walkPhase;
        walkPhase += moved * 0.12f;
        if (std::floor(walkPhase / (2.f * PI)) != std::floor(before / (2.f * PI))) cue(Sfx::Footstep, w.playerPos, 1.f, rnd(0.9f, 1.1f));
        // HUD-level events
        if (w.inventory.count("Medkit") < prevMedkits_) cue(Sfx::Medkit, w.playerPos);
        if (w.weapon != prevWeapon_) cue(Sfx::Switch, w.playerPos);
        if (w.flashlight != prevFlash_) cue(Sfx::Torch, w.playerPos);
        if (w.waveNumber > prevWave_) cue(Sfx::Wave, w.playerPos);
        if ((w.objectivesDone & ~prevObjectives_) != 0u) cue(Sfx::Objective, w.playerPos);
        if (w.mission.goalUnlocked() && !prevUnlocked_) cue(Sfx::Unlock, w.playerPos);
        moveAmt += ((moved > 0.2f ? 1.f : 0.f) - moveAmt) * clamp01(dt * 10.f);
        snapshot(w);
    }

    shake *= std::exp(-9.f * dt);
    muzzle = std::max(0.f, muzzle - dt);
    recoil = std::max(0.f, recoil - dt * 9.f);
    hurt = std::max(0.f, hurt - dt * 1.5f);
    muzzle2 = std::max(0.f, muzzle2 - dt);
    recoil2 = std::max(0.f, recoil2 - dt * 9.f);
    hurt2 = std::max(0.f, hurt2 - dt * 1.5f);
    for (Particle& p : parts) {
        p.p += p.v * dt;
        p.v = p.v * std::exp(-p.drag * dt);
        p.life -= dt;
        p.size = std::max(0.2f, p.size + p.grow * dt);
        p.rot += p.spin * dt;
        p.spin *= std::exp(-3.f * dt);
        if (p.life <= 0.f) {                                               
            if (p.type == PType::Blood && rnd(0, 1) < 0.35f) decal(DType::Drop, p.p, rnd(5, 10), 40.f, (int)rnd(0, 2.99f));
            if (p.type == PType::Shell) { decal(DType::Shell, p.p, 1.f, 10.f, 0, p.rot * 180.f / PI); cue(Sfx::Tinkle, p.p, rnd(0.5f, 1.f), rnd(0.9f, 1.2f)); }
        }
    }
    parts.erase(std::remove_if(parts.begin(), parts.end(), [](const Particle& p) { return p.life <= 0.f; }), parts.end());
    for (Decal& d : decals) d.age += dt;
    decals.erase(std::remove_if(decals.begin(), decals.end(), [](const Decal& d) { return d.age > d.life; }), decals.end());
    for (Popup& p : popups) { p.t += dt; p.p.y -= 34.f * dt; }
    popups.erase(std::remove_if(popups.begin(), popups.end(), [](const Popup& p) { return p.t > 1.1f; }), popups.end());
}
