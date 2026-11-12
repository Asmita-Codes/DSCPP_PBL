#include "Renderer.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float RAD2DEG = 180.f / PI;
inline float hashf(int x, int y) {
    unsigned h = (unsigned)x * 73856093u ^ (unsigned)y * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (float)(h & 0xffff) / 65536.f;
}
Col pickupGlow(PickupKind k) {
    switch (k) {
    case PickupKind::Health: return Col(255, 90, 90);
    case PickupKind::Ammo: return Col(255, 210, 90);
    case PickupKind::Grenade: return Col(140, 230, 110);
    case PickupKind::Fuel: return Col(255, 140, 50);
    }
    return C::White;
}
}  

WorldRenderer::~WorldRenderer() {
    if (light_) SDL_DestroyTexture(light_);
    if (mini_) SDL_DestroyTexture(mini_);
}

bool WorldRenderer::init(Gfx& g) {
    if (light_) SDL_DestroyTexture(light_);
    if (mini_) SDL_DestroyTexture(mini_);
    light_ = g.makeTarget((int)UI_W, (int)UI_H);
    mini_ = SDL_CreateTexture(g.ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, COLS, ROWS);
    if (mini_) { SDL_SetTextureScaleMode(mini_, SDL_ScaleModeNearest); SDL_SetTextureBlendMode(mini_, SDL_BLENDMODE_BLEND); }
    miniVersion_ = ~0u;
    return light_ != nullptr;
}


void WorldRenderer::resetCamera(const Game& w) {
    
    Vec2 f = focus(w);
    cam_ = Vec2(std::clamp(f.x - VIEW_W / 2, 0.f, WORLD_W - VIEW_W), std::clamp(f.y - VIEW_H / 2, 0.f, WORLD_H - VIEW_H));
}

void WorldRenderer::updateCamera(const Game& w, float dt) {
    Vec2 look = w.coop ? focus(w) : w.playerPos + w.aimDir * 70.f;   
    Vec2 target(std::clamp(look.x - VIEW_W / 2, 0.f, WORLD_W - VIEW_W), std::clamp(look.y - VIEW_H / 2, 0.f, WORLD_H - VIEW_H));
    float k = 1.f - std::exp(-7.f * dt);
    cam_ = cam_ + (target - cam_) * k;
}

Vec2 WorldRenderer::focus(const Game& w) const {
    if (!w.coop) return w.playerPos;
    return (w.playerPos + w.p2.pos) * 0.5f;                     
}


void WorldRenderer::drawGround(Gfx& g, const Art& a, Vec2 tl) {
    for (const Art::Chunk& c : a.ground) {
        if (c.x + c.w < tl.x || c.x > tl.x + UI_W || c.y + c.h < tl.y || c.y > tl.y + UI_H) continue;
        g.image(c.tex, c.x, c.y, c.w + 0.6f, c.h + 0.6f, C::White, SDL_BLENDMODE_NONE);   
    }
}

void WorldRenderer::lampLights(const Art& a, double t, Vec2 tl) {
    for (const Art::Lamp& l : a.lamps) {
        if (l.mode == 2 || !visible(Vec2(l.x, l.y), 230.f, tl)) continue;
        float k = 1.f;
        if (l.mode == 1) {                                          
            float f = (float)(std::sin(t * 13.0 + l.seed) + std::sin(t * 7.3 + l.seed * 2.0) + std::sin(t * 31.0 + l.seed));
            k = f > 1.2f ? 0.15f : 0.85f + 0.15f * (float)std::sin(t * 50.0);
        }
        lights_.push_back({l.x + 8.f, l.y + 8.f, 210.f, Col(255, 176, 104).scaled(0.62f * k)});
    }
}

void WorldRenderer::lightPass(Gfx& g, const Art& a, Vec2 tl, Col ambient) {
    if (!light_) { lights_.clear(); cones_.clear(); return; }
    SDL_SetRenderTarget(g.ren, light_);
    SDL_SetRenderDrawColor(g.ren, ambient.r, ambient.g, ambient.b, 255);
    SDL_RenderClear(g.ren);
    g.setOrigin(tl.x, tl.y);
    for (const Light& l : lights_)
        if (visible(Vec2(l.x, l.y), l.r, tl)) g.sprite(a.glow, l.x, l.y, l.r * 2.f, l.r * 2.f, 0.f, l.c, SDL_BLENDMODE_ADD);
    for (const Cone& c : cones_) {
        const float len = 420.f;
        g.spritePivot(a.cone, c.pos.x, c.pos.y, len, len * 480.f / 512.f, 0.f, 0.5f, c.ang * RAD2DEG, Col(255, 244, 214), SDL_BLENDMODE_ADD);
        g.spritePivot(a.cone, c.pos.x, c.pos.y, len * 0.55f, len * 0.55f * 480.f / 512.f, 0.f, 0.5f, c.ang * RAD2DEG, Col(120, 115, 100), SDL_BLENDMODE_ADD);
    }
    cones_.clear();
    SDL_SetRenderTarget(g.ren, nullptr);
    g.ui();
    g.image(light_, 0, 0, UI_W, UI_H, C::White, SDL_BLENDMODE_MOD);        
    g.setOrigin(tl.x, tl.y);
    lights_.clear();
}


void WorldRenderer::draw(Gfx& g, const Art& a, const Game& w, const Fx& fx, bool debugPaths) {
    const double t = g.time;
    const float ft = (float)t;
    Vec2 tl = cam_ + fx.shakeOffset(t);
    g.setOrigin(tl.x, tl.y);
    drawGround(g, a, tl);

    const bool zvOk = fx.zv.size() == w.zombies.size();
    const bool pvOk = fx.pv.size() == w.pickups.size();
    const bool gvOk = fx.gv.size() == w.grenades.size();

   
    for (const Decal& d : fx.decals) {
        if (!visible(d.p, d.size * 2.f, tl)) continue;
        float fade = clamp01((d.life - d.age) / 6.f) * clamp01(d.age / 0.15f + 0.3f);
        switch (d.type) {
        case DType::Scorch: g.sprite(a.scorch, d.p.x, d.p.y, d.size, d.size, d.rot, Col(255, 255, 255, alpha8(235 * fade))); break;
        case DType::Blood: {
            float grow = 0.55f + 0.45f * clamp01(d.age / 0.5f);                 // pools spread out
            g.sprite(a.blood[d.var % 3], d.p.x, d.p.y, d.size * grow, d.size * grow, d.rot, Col(255, 255, 255, alpha8(230 * fade)));
            break;
        }
        case DType::Drop: g.sprite(a.blood[d.var % 3], d.p.x, d.p.y, d.size, d.size, d.rot, Col(255, 255, 255, alpha8(200 * fade))); break;
        case DType::Acid: g.sprite(a.soft, d.p.x, d.p.y, d.size, d.size, 0, Col(120, 200, 40, alpha8(150 * fade))); break;
        case DType::Shell: {
            float r = d.rot / RAD2DEG;
            g.line(d.p.x - std::cos(r) * 2.f, d.p.y - std::sin(r) * 2.f, d.p.x + std::cos(r) * 2.f, d.p.y + std::sin(r) * 2.f, 1.6f, Col(200, 160, 70, alpha8(255 * fade)));
            break;
        }
        case DType::Corpse: {
            float s = Art::zombieSize(d.size);
            g.sprite(a.zombie[std::clamp(d.var, 0, 4)], d.p.x, d.p.y, s * 0.95f, s * 0.95f, d.rot, Col(110, 92, 90, alpha8(255 * fade)));
            break;
        }
        }
    }

    
    int x0 = std::max(0, (int)(tl.x / TILE) - 1), x1 = std::min(COLS - 1, (int)((tl.x + UI_W) / TILE) + 1);
    int y0 = std::max(0, (int)(tl.y / TILE) - 1), y1 = std::min(ROWS - 1, (int)((tl.y + UI_H) / TILE) + 1);
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            TileType tt = w.graph.tile(Vec2i{x, y});
            Vec2 c = tileCenter(Vec2i{x, y});
            if (tt == TileType::Rubble) {
                float h = hashf(x, y);
                g.sprite(a.rubble[(int)(h * 3.f) % 3], c.x, c.y, 38, 38, std::floor(h * 4.f) * 90.f);
            } else if (tt == TileType::Barricade) {
                float frac = w.barricadeFraction(Vec2i{x, y});
                bool chewed = false;
                for (const Zombie& z : w.zombies) if (z.attackTile == Vec2i{x, y}) { chewed = true; break; }
                float jx = chewed ? (float)std::sin(t * 60.0 + x) * 1.2f : 0.f;
                g.sprite(a.soft, c.x + 4, c.y + 6, 46, 46, 0, Col(0, 0, 0, 120));
                Col mod = mixCol(Col(130, 110, 100), C::White, frac);
                g.sprite(a.barricade, c.x + jx, c.y, 33, 33, 0, mod);
                g.sprite(a.crack, c.x + jx, c.y, 33, 33, 0, Col(255, 255, 255, alpha8((1.f - frac) * 300.f)));
            }
        }

    
    const MissionDef& md = w.mission.def();
    const bool unlocked = w.mission.goalUnlocked();
    const Vec2 gp = w.mission.goalPos();
    const Col zc = unlocked ? Col(80, 240, 140) : Col(240, 90, 60);
    const float pulse = 0.5f + 0.5f * (float)std::sin(t * (unlocked ? 4.0 : 2.0));
    if (md.type == MissionType::SurviveThenReach) {                       
        g.circle(gp.x, gp.y, 46, Col(30, 34, 40, 200));
        g.ring(gp.x, gp.y, 44, 3, Col(235, 235, 235, 200));
        g.rect(gp.x - 15, gp.y - 18, 6, 36, Col(240, 240, 240, 220));
        g.rect(gp.x + 9, gp.y - 18, 6, 36, Col(240, 240, 240, 220));
        g.rect(gp.x - 9, gp.y - 3, 18, 6, Col(240, 240, 240, 220));
    } else if (md.type == MissionType::CollectFuel) {                     
        g.rect(gp.x - 40, gp.y - 40, 80, 80, Col(36, 34, 30, 200));
        for (int i = 0; i < 8; ++i) {
            float o = i * 10.f;
            Col sc = (i % 2) ? Col(250, 200, 40, 220) : Col(30, 30, 30, 220);
            g.rect(gp.x - 40 + o, gp.y - 40, 10, 6, sc); g.rect(gp.x - 40 + o, gp.y + 34, 10, 6, sc);
            g.rect(gp.x - 40, gp.y - 40 + o, 6, 10, sc); g.rect(gp.x + 34, gp.y - 40 + o, 6, 10, sc);
        }
    } else {                                                             
        g.roundRect(gp.x - 40, gp.y - 40, 80, 80, 10, zc.alpha(0.18f));
    }
    
    for (int i = 0; i < 24; ++i) {
        float a0 = (i / 24.f) * 2 * PI + ft * (unlocked ? 0.8f : 0.3f), a1 = a0 + 0.14f;
        g.arc(gp.x, gp.y, 54, 3, a0, a1, zc.alpha(0.55f + 0.45f * pulse));
    }

    
    for (size_t i = 0; i < w.pickups.size(); ++i) {
        const Pickup& k = w.pickups[i];
        if (!visible(k.pos, 30, tl)) continue;
        float age = pvOk ? fx.pv[i].age : 9.f;
        float pop = age < 0.4f ? 1.f + 0.6f * std::sin(age / 0.4f * PI) * (1.f - age / 0.4f) : 1.f;
        pop *= std::min(1.f, 0.3f + age * 4.f);
        float bob = std::sin(ft * 3.f + k.pos.x * 0.05f) * 2.5f;
        float s = (k.kind == PickupKind::Fuel ? 30.f : 26.f) * pop;
        g.sprite(a.soft, k.pos.x + 2, k.pos.y + 7, s * 1.2f, s * 0.7f, 0, Col(0, 0, 0, 130));
        g.sprite(a.pickup[(int)k.kind], k.pos.x, k.pos.y - 3 + bob, s, s, std::sin(ft * 2.f + k.pos.y) * 6.f);
    }

    
    for (size_t i = 0; i < w.grenades.size(); ++i) {
        const Grenade& gr = w.grenades[i];
        float prog = gvOk ? 1.f - clamp01((gr.pos - gr.target).length() / fx.gv[i].startDist) : 1.f;
        float h = std::sin(prog * PI) * 30.f;
        g.sprite(a.soft, gr.pos.x + 2, gr.pos.y + 4, 16 - h * 0.15f, 10 - h * 0.1f, 0, Col(0, 0, 0, 140));
        g.sprite(a.pickup[(int)PickupKind::Grenade], gr.pos.x, gr.pos.y - h, 15 + h * 0.15f, 15 + h * 0.15f, ft * 600.f * (1.f - prog));
    }

    
    for (size_t i = 0; i < w.zombies.size(); ++i) {
        const Zombie& z = w.zombies[i];
        float sz = Art::zombieSize(z.radius);
        if (!visible(z.pos, sz, tl)) continue;
        float heading = zvOk ? fx.zv[i].heading : std::atan2(w.playerPos.y - z.pos.y, w.playerPos.x - z.pos.x);
        float phase = zvOk ? fx.zv[i].phase : 0.f, flash = zvOk ? fx.zv[i].flash : 0.f, age = zvOk ? fx.zv[i].age : 9.f;
        float rise = clamp01(age / 0.6f);
        float sway = std::sin(phase) * 7.f;
        g.sprite(a.soft, z.pos.x + 4, z.pos.y + 6, z.radius * 3.4f, z.radius * 3.0f, 0, Col(0, 0, 0, alpha8(140 * rise)));
        if (z.kind == ZombieKind::Boss && z.charging > 0.f) {
            for (int k = 1; k <= 3; ++k) {                              // motion trail
                Vec2 back = z.pos - Vec2(std::cos(heading), std::sin(heading)) * (k * 10.f);
                g.sprite(a.zombie[4], back.x, back.y, sz, sz, heading * RAD2DEG, Col(255, 120, 80, alpha8(70 - k * 18)));
            }
        }
        float s = sz * (0.6f + 0.4f * rise) * (1.f + 0.03f * std::sin(phase * 2.f));
        g.sprite(a.zombie[(int)z.kind], z.pos.x, z.pos.y, s, s, heading * RAD2DEG + sway, Col(255, 255, 255, alpha8(255 * rise)));
        if (flash > 0.f) g.sprite(a.zombie[(int)z.kind], z.pos.x, z.pos.y, s, s, heading * RAD2DEG + sway, Col(255, 255, 255, alpha8(flash * 220)), SDL_BLENDMODE_ADD);
    }

    
    const float aim = std::atan2(w.aimDir.y, w.aimDir.x);
    const Vec2 pp = w.playerPos;
    const Vec2 kick = w.aimDir * (-fx.recoil * 3.f);
    g.sprite(a.soft, pp.x + 4, pp.y + 6, 44, 40, 0, Col(0, 0, 0, 150));     // the players themselves are drawn after the lighting
    if (w.coop) g.sprite(a.soft, w.p2.pos.x + 4, w.p2.pos.y + 6, 44, 40, 0, Col(0, 0, 0, 150));
    const float aim2 = std::atan2(w.p2.aimDir.y, w.p2.aimDir.x);
    const Vec2 kick2 = w.p2.aimDir * (-fx.recoil2 * 3.f);

   
    for (const Acid& ac : w.acids) g.circle(ac.pos.x, ac.pos.y, 4.5f, Col(120, 200, 40));

    
    for (const Particle& p : fx.parts) {
        float k = clamp01(p.life / p.maxLife);
        switch (p.type) {
        case PType::Blood: g.circle(p.p.x, p.p.y, p.size, p.c.alpha(std::min(1.f, k * 2.f))); break;
        case PType::Smoke: g.sprite(a.smoke, p.p.x, p.p.y, p.size * 2.f, p.size * 2.f, p.rot * RAD2DEG, p.c.alpha(k * 0.75f)); break;
        case PType::Dust: g.sprite(a.smoke, p.p.x, p.p.y, p.size * 2.f, p.size * 2.f, p.rot * RAD2DEG, p.c.alpha(k * 0.45f)); break;
        case PType::Chip: {
            float c = std::cos(p.rot) * p.size, s = std::sin(p.rot) * p.size;
            g.line(p.p.x - c, p.p.y - s, p.p.x + c, p.p.y + s, p.size * 0.8f, p.c.alpha(std::min(1.f, k * 3.f)));
            break;
        }
        case PType::Shell: {
            float c = std::cos(p.rot) * 2.2f, s = std::sin(p.rot) * 2.2f;
            g.line(p.p.x - c, p.p.y - s, p.p.x + c, p.p.y + s, 1.8f, p.c);
            break;
        }
        default: break;
        }
    }

   
    for (int layer = 0; layer < 2; ++layer) {
        float sc = layer ? 900.f : 640.f, sp = layer ? 9.f : 14.f;
        float ox = std::fmod(ft * sp + layer * 300.f, sc), oy = std::fmod(ft * sp * 0.4f + layer * 170.f, sc);
        float bx = std::floor(tl.x / sc) * sc - ox, by = std::floor(tl.y / sc) * sc - oy;
        for (float yy = by; yy < tl.y + UI_H; yy += sc)
            for (float xx = bx; xx < tl.x + UI_W; xx += sc)
                if (xx + sc > tl.x && yy + sc > tl.y) g.image(a.fog, xx, yy, sc, sc, Col(200, 210, 230, layer ? 26 : 34));
    }

    
    lampLights(a, t, tl);
    if (!w.p1Down) lights_.push_back({pp.x, pp.y, 150.f, Col(120, 120, 128)});
    if (w.coop && !w.p2Down) lights_.push_back({w.p2.pos.x, w.p2.pos.y, 150.f, Col(120, 120, 128)});
    if (w.coop && fx.muzzle2 > 0.f) lights_.push_back({w.p2.pos.x + w.p2.aimDir.x * 24, w.p2.pos.y + w.p2.aimDir.y * 24, 260.f, Col(255, 200, 120).scaled(fx.muzzle2 / 0.07f)});
    if (fx.muzzle > 0.f) lights_.push_back({pp.x + w.aimDir.x * 24, pp.y + w.aimDir.y * 24, 260.f, Col(255, 200, 120).scaled(fx.muzzle / 0.07f)});
    for (const Bullet& b : w.bullets) lights_.push_back({b.pos.x, b.pos.y, 60.f, Col(160, 130, 90)});
    for (const Acid& ac : w.acids) lights_.push_back({ac.pos.x, ac.pos.y, 70.f, Col(90, 180, 40)});
    for (const Explosion& e : w.explosions) {
        float k = clamp01(e.t / 0.45f);
        lights_.push_back({e.pos.x, e.pos.y, 480.f * (1.f - 0.4f * k), Col(255, 170, 80).scaled(1.f - k * k)});
    }
    for (const Particle& p : fx.parts) if (p.type == PType::Ember && p.life > 0.2f && lights_.size() < 160) lights_.push_back({p.p.x, p.p.y, 40.f, Col(120, 70, 20)});
    lights_.push_back({gp.x, gp.y, 230.f, zc.scaled(0.45f + 0.35f * pulse)});
    for (const Pickup& k : w.pickups) lights_.push_back({k.pos.x, k.pos.y, 56.f, pickupGlow(k.kind).scaled(0.45f)});
    for (const Zombie& z : w.zombies) {
        if (z.kind == ZombieKind::Spitter) lights_.push_back({z.pos.x, z.pos.y, 60.f, Col(60, 110, 20)});
        if (z.kind == ZombieKind::Boss) lights_.push_back({z.pos.x, z.pos.y, z.charging > 0.f ? 200.f : 110.f, Col(160, 40, 30)});
    }
    for (size_t i = 0; i < w.grenades.size(); ++i) {
        bool on = std::fmod(w.grenades[i].fuse * (w.grenades[i].fuse < 0.4f ? 14.f : 6.f), 1.f) < 0.5f;
        if (on) lights_.push_back({w.grenades[i].pos.x, w.grenades[i].pos.y, 50.f, Col(200, 30, 20)});
    }
    if (w.flashlight && !w.p1Down) cones_.push_back({pp + w.aimDir * 10.f, aim});
    if (w.coop && w.flashlight && !w.p2Down) cones_.push_back({w.p2.pos + w.p2.aimDir * 10.f, aim2});   
    lightPass(g, a, tl, Col(88, 96, 128));

    
    if (w.coop) drawSurvivor(g, a, 1, true, w.p2.pos + kick2, w.p2.aimDir, w.p2.weapon, fx.walkPhase2, fx.moveAmt2, fx.recoil2, fx.hurt2, w.p2Down, w.p2Revive);
    drawSurvivor(g, a, 0, w.coop, pp + kick, w.aimDir, w.weapon, fx.walkPhase, fx.moveAmt, fx.recoil, fx.hurt, w.p1Down, w.p1Revive);

    
    g.setBlend(SDL_BLENDMODE_ADD);
    for (const Art::Lamp& l : a.lamps) {
        if (l.mode == 2 || !visible(Vec2(l.x, l.y), 30, tl)) continue;
        g.sprite(a.glow, l.x, l.y, 26, 26, 0, Col(255, 200, 130, 200), SDL_BLENDMODE_ADD);
        g.circle(l.x, l.y, 2.f, Col(255, 240, 210));
    }
    for (const Bullet& b : w.bullets) {
        Vec2 tail = b.pos - b.vel * 0.035f;
        g.lineGrad(tail.x, tail.y, b.pos.x, b.pos.y, 2.4f, Col(255, 180, 80, 0), Col(255, 240, 190, 255));
        g.sprite(a.glow, b.pos.x, b.pos.y, 14, 14, 0, Col(255, 210, 140, 200), SDL_BLENDMODE_ADD);
    }
    if (w.coop && fx.muzzle2 > 0.f) {
        Vec2 m = w.p2.pos + kick2 + w.p2.aimDir * (w.p2.weapon == 0 ? 30.f : 22.f);
        float k = fx.muzzle2 / 0.07f;
        g.sprite(a.glow, m.x, m.y, 46 * k, 46 * k, 0, Col(255, 190, 90), SDL_BLENDMODE_ADD);
        g.sprite(a.glow, m.x + w.p2.aimDir.x * 10, m.y + w.p2.aimDir.y * 10, 44 * k, 14 * k, aim2 * RAD2DEG, Col(255, 240, 200), SDL_BLENDMODE_ADD);
    }
    if (fx.muzzle > 0.f) {
        Vec2 m = pp + kick + w.aimDir * (w.weapon == 0 ? 30.f : 22.f);
        float k = fx.muzzle / 0.07f;
        g.sprite(a.glow, m.x, m.y, 46 * k, 46 * k, 0, Col(255, 190, 90), SDL_BLENDMODE_ADD);
        g.sprite(a.glow, m.x + w.aimDir.x * 10, m.y + w.aimDir.y * 10, 44 * k, 14 * k, aim * RAD2DEG, Col(255, 240, 200), SDL_BLENDMODE_ADD);
        g.sprite(a.glow, m.x, m.y, 12, 12, 0, C::White, SDL_BLENDMODE_ADD);
    }
    for (const Acid& ac : w.acids) g.sprite(a.glow, ac.pos.x, ac.pos.y, 24, 24, 0, Col(150, 255, 60, 220), SDL_BLENDMODE_ADD);
    for (const Explosion& e : w.explosions) {
        float k = clamp01(e.t / 0.45f);
        g.sprite(a.glow, e.pos.x, e.pos.y, 260 * (0.4f + k), 260 * (0.4f + k), 0, Col(255, 150, 60, alpha8(255 * (1 - k))), SDL_BLENDMODE_ADD);
        g.sprite(a.glow, e.pos.x, e.pos.y, 120 * (1 - k * 0.5f), 120 * (1 - k * 0.5f), 0, Col(255, 240, 200, alpha8(255 * (1 - k))), SDL_BLENDMODE_ADD);
        g.ring(e.pos.x, e.pos.y, 20 + 110 * k, 6 * (1 - k) + 1, Col(255, 220, 160, alpha8(200 * (1 - k))));
    }
    for (const Particle& p : fx.parts) {
        float k = clamp01(p.life / p.maxLife);
        if (p.type == PType::Spark) {
            Vec2 tail = p.p - p.v * 0.03f;
            g.lineGrad(tail.x, tail.y, p.p.x, p.p.y, p.size, p.c.alpha(0.f), p.c.alpha(k));
        } else if (p.type == PType::Ember || p.type == PType::Glint || p.type == PType::Acid) {
            float s = p.size * (p.type == PType::Ember ? 4.f : 3.f);
            g.sprite(a.glow, p.p.x, p.p.y, s, s, 0, p.c.alpha(k), SDL_BLENDMODE_ADD);
        }
    }
    for (size_t i = 0; i < w.grenades.size(); ++i) {
        const Grenade& gr = w.grenades[i];
        float prog = gvOk ? 1.f - clamp01((gr.pos - gr.target).length() / fx.gv[i].startDist) : 1.f;
        bool on = std::fmod(gr.fuse * (gr.fuse < 0.4f ? 14.f : 6.f), 1.f) < 0.5f;
        if (on) g.sprite(a.glow, gr.pos.x, gr.pos.y - std::sin(prog * PI) * 30.f, 16, 16, 0, Col(255, 60, 40), SDL_BLENDMODE_ADD);
    }
    for (size_t i = 0; i < w.pickups.size(); ++i) {                    
        const Pickup& k = w.pickups[i];
        if (!visible(k.pos, 40, tl)) continue;
        float pl = 0.6f + 0.4f * std::sin(ft * 4.f + k.pos.x);
        g.sprite(a.glow, k.pos.x, k.pos.y, 48, 48, 0, pickupGlow(k.kind).alpha(0.22f * pl), SDL_BLENDMODE_ADD);
    }
    g.sprite(a.glow, gp.x, gp.y, 150, 150, 0, zc.alpha(0.12f + 0.12f * pulse), SDL_BLENDMODE_ADD);
    g.setBlend(SDL_BLENDMODE_BLEND);

    
    for (size_t i = 1; i + 1 < w.route.size(); ++i) {
        Vec2 c = tileCenter(w.route[i]), n = tileCenter(w.route[i + 1]);
        Vec2 d = n - c;
        float l = d.length();
        if (l < 0.1f || !visible(c, 20, tl)) continue;
        d = d / l;
        Vec2 side(-d.y, d.x);
        float wave = 0.5f + 0.5f * std::sin(ft * 7.f - (float)i * 0.9f);
        Col cc = Col(255, 205, 80, alpha8(70 + 150 * wave));
        Vec2 tip = c + d * 5.f, b1 = c - d * 2.f + side * 5.f, b2 = c - d * 2.f - side * 5.f;
        g.line(b1.x, b1.y, tip.x, tip.y, 2.4f, cc);
        g.line(b2.x, b2.y, tip.x, tip.y, 2.4f, cc);
    }

    
    for (const Grenade& gr : w.grenades) {
        for (int i = 0; i < 20; ++i) {
            float a0 = i / 20.f * 2 * PI + ft, a1 = a0 + 0.17f;
            g.arc(gr.target.x, gr.target.y, 100, 1.6f, a0, a1, Col(255, 120, 60, 150));
        }
    }

    
    if (debugPaths)
        for (const Zombie& z : w.zombies) {
            if (z.path.size() < 2) continue;
            Col pc = z.attackTile.x >= 0 ? Col(255, 170, 60, 200) : Col(255, 80, 80, 190);
            Vec2 prev = z.pos;
            for (size_t i = 1; i < z.path.size(); ++i) {
                Vec2 c = tileCenter(z.path[i]);
                g.line(prev.x, prev.y, c.x, c.y, 1.6f, pc);
                prev = c;
            }
            g.circle(prev.x, prev.y, 3.f, pc);
        }

    
    for (size_t i = 0; i < w.zombies.size(); ++i) {
        const Zombie& z = w.zombies[i];
        if (!visible(z.pos, 40, tl)) continue;
        bool boss = z.kind == ZombieKind::Boss;
        if (z.hp < z.maxHp || boss) {
            float bw = boss ? 64.f : z.radius * 2.4f, by = z.pos.y - z.radius - 14.f;
            g.roundRect(z.pos.x - bw / 2 - 1, by - 1, bw + 2, 6, 3, Col(0, 0, 0, 170));
            g.roundRect(z.pos.x - bw / 2, by, bw * clamp01(z.hp / z.maxHp), 4, 2, boss ? Col(255, 90, 60) : Col(255, 210, 90), boss ? Col(200, 30, 20) : Col(230, 120, 40));
        }
        if (z.alert > 0.f) {
            float bob = std::sin(ft * 10.f) * 1.5f;
            float ay = z.pos.y - z.radius - (z.hp < z.maxHp || boss ? 32.f : 24.f) + bob;
            g.circle(z.pos.x, ay + 7, 7.f, Col(255, 200, 60, 230));
            g.text("!", z.pos.x, ay - 1, 12, Col(40, 20, 0), Align::Center, Font::Title);
        }
    }

    
    for (const Popup& p : fx.popups) {
        float k = clamp01(1.f - (p.t - 0.6f) / 0.5f);
        float sc = p.t < 0.12f ? 0.7f + p.t / 0.12f * 0.5f : 1.2f - std::min(0.2f, (p.t - 0.12f) * 0.8f);
        g.textShadow(p.s, p.p.x, p.p.y - 10, 15 * sc, p.c.alpha(k), Align::Center, Font::Title, 1.5f);
    }

    
    std::string label = unlocked ? std::string(md.zoneName) : std::string(md.zoneName) + "  -  LOCKED";
    float lw = g.textWidth(label, 13, Font::Title);
    g.roundRect(gp.x - lw / 2 - 10, gp.y - 84, lw + 20, 24, 12, Col(10, 14, 18, 190));
    g.roundOutline(gp.x - lw / 2 - 10, gp.y - 84, lw + 20, 24, 12, zc.alpha(0.8f));
    g.textMid(label, gp.x, gp.y - 72, 13, zc, Align::Center, Font::Title);
    g.ui();
}


void WorldRenderer::drawSurvivor(Gfx& g, const Art& a, int team, bool tag, Vec2 pos, Vec2 aimDir, int weapon, float walkPhase,
                                 float moveAmt, float recoil, float hurt, bool down, float revive) {
    (void)recoil;
    const Col tc = team == 0 ? Col(90, 205, 255) : Col(255, 110, 200);    
    const float ps = Art::playerSize();
    SDL_Texture* tex = a.player[team][weapon == 0 ? 0 : 1];
    if (down) {                                                   
        g.sprite(tex, pos.x, pos.y, ps, ps, 90.f, Col(120, 120, 130));
        g.ring(pos.x, pos.y, 26.f, 3.f, Col(0, 0, 0, 120));
        g.arc(pos.x, pos.y, 26.f, 3.f, -PI / 2, -PI / 2 + 2 * PI * revive, tc);
        float tw = g.textWidth("DOWN - STAND CLOSE TO REVIVE", 10, Font::Title) + 16;
        g.roundRect(pos.x - tw / 2, pos.y - 52, tw, 18, 9, Col(150, 20, 20, 230));
        g.textMid("DOWN - STAND CLOSE TO REVIVE", pos.x, pos.y - 43, 10, C::White, Align::Center, Font::Title);
        return;
    }
    const float aim = std::atan2(aimDir.y, aimDir.x);
    const float sway = std::sin(walkPhase) * 3.f * moveAmt;
    g.sprite(a.glow, pos.x, pos.y, 84, 84, 0, tc.alpha(0.22f), SDL_BLENDMODE_ADD);   
    g.ring(pos.x, pos.y, 23.f, 2.4f, tc.alpha(0.85f));                             
    g.sprite(tex, pos.x, pos.y, ps, ps, aim * RAD2DEG + sway);
    if (hurt > 0.05f) g.sprite(tex, pos.x, pos.y, ps, ps, aim * RAD2DEG + sway, Col(255, 60, 60, alpha8(hurt * 255)), SDL_BLENDMODE_ADD);
    if (tag) {                                                    
        const char* name = team == 0 ? "P1" : "P2";
        g.roundRect(pos.x - 13, pos.y - 46, 26, 16, 8, tc);
        g.textMid(name, pos.x, pos.y - 38, 10.5f, Col(10, 16, 28), Align::Center, Font::Title);
    }
}


void WorldRenderer::drawBackdrop(Gfx& g, const Art& a) {
    const double t = g.time;
    Vec2 tl(320.f + 300.f * (float)std::sin(t * 0.045), 280.f + 260.f * (float)std::sin(t * 0.031 + 1.0));
    g.setOrigin(tl.x, tl.y);
    drawGround(g, a, tl);
    for (int layer = 0; layer < 2; ++layer) {
        float sc = layer ? 900.f : 640.f, ft = (float)t;
        float ox = std::fmod(ft * 12.f + layer * 300.f, sc);
        float bx = std::floor(tl.x / sc) * sc - ox, by = std::floor(tl.y / sc) * sc;
        for (float yy = by; yy < tl.y + UI_H; yy += sc)
            for (float xx = bx; xx < tl.x + UI_W; xx += sc) g.image(a.fog, xx, yy, sc, sc, Col(200, 210, 230, 40));
    }
    lampLights(a, t, tl);
    lightPass(g, a, tl, Col(70, 76, 104));
    g.setBlend(SDL_BLENDMODE_ADD);
    for (const Art::Lamp& l : a.lamps)
        if (l.mode != 2 && visible(Vec2(l.x, l.y), 30, tl)) g.sprite(a.glow, l.x, l.y, 26, 26, 0, Col(255, 200, 130, 200), SDL_BLENDMODE_ADD);
    g.setBlend(SDL_BLENDMODE_BLEND);
    g.ui();
    g.image(a.vignette, 0, 0, UI_W, UI_H, Col(0, 0, 0, 255));
    g.rectGrad(0, 0, UI_W, UI_H, Col(8, 10, 16, 120), Col(8, 10, 16, 200));
}


void WorldRenderer::drawMinimap(Gfx& g, const Game& w, float x, float y, float wd, float ht) {
    if (mini_ && miniVersion_ != w.graph.version()) {              
        std::vector<Uint8> px((size_t)COLS * ROWS * 4);
        for (int ty = 0; ty < ROWS; ++ty)
            for (int tx = 0; tx < COLS; ++tx) {
                TileType tt = w.graph.tile(Vec2i{tx, ty});
                Col c = tt == TileType::Building ? Col(30, 36, 46) : tt == TileType::Road ? Col(88, 96, 110)
                      : tt == TileType::Rubble ? Col(130, 108, 76) : Col(235, 150, 50);
                Uint8* p = &px[((size_t)ty * COLS + tx) * 4];
                p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = 235;
            }
        SDL_UpdateTexture(mini_, nullptr, px.data(), COLS * 4);
        miniVersion_ = w.graph.version();
    }
    const float ft = (float)g.time;
    g.roundRect(x - 3, y - 3, wd + 6, ht + 6, 6, Col(6, 8, 12, 230));
    g.roundOutline(x - 3, y - 3, wd + 6, ht + 6, 6, C::PanelLine);
    g.image(mini_, x, y, wd, ht);
    auto M = [&](Vec2 p) { return FPt{x + p.x / WORLD_W * wd, y + p.y / WORLD_H * ht}; };
    
    FPt c0 = M(cam_), c1 = M(cam_ + Vec2(VIEW_W, VIEW_H));
    g.outline(c0.x, c0.y, c1.x - c0.x, c1.y - c0.y, Col(255, 255, 255, 70), 1.f);
   
    for (size_t i = 1; i < w.route.size(); ++i) {
        FPt a = M(tileCenter(w.route[i - 1])), b = M(tileCenter(w.route[i]));
        g.line(a.x, a.y, b.x, b.y, 1.5f, Col(255, 205, 80, 160));
    }
    for (const Pickup& p : w.pickups)
        if (p.kind == PickupKind::Fuel) { FPt m = M(p.pos); g.circle(m.x, m.y, 3.f, Col(255, 150, 50)); }
    for (const Zombie& z : w.zombies) {
        FPt m = M(z.pos);
        if (z.kind == ZombieKind::Boss) { g.circle(m.x, m.y, 4.5f + std::sin(ft * 8.f), Col(255, 60, 40)); }
        else g.circle(m.x, m.y, 1.8f, Col(255, 80, 70, 220));
    }
    FPt gpos = M(w.mission.goalPos());
    Col zc = w.mission.goalUnlocked() ? Col(90, 240, 140) : Col(240, 110, 70);
    g.ring(gpos.x, gpos.y, 5.f + 2.f * std::sin(ft * 4.f), 1.6f, zc);
    g.circle(gpos.x, gpos.y, 2.5f, zc);
    if (w.coop) {                                                 
        FPt q = M(w.p2.pos);
        float a2 = std::atan2(w.p2.aimDir.y, w.p2.aimDir.x);
        g.tri(FPt{q.x + std::cos(a2) * 7.f, q.y + std::sin(a2) * 7.f}, FPt{q.x + std::cos(a2 + 2.5f) * 5.f, q.y + std::sin(a2 + 2.5f) * 5.f},
              FPt{q.x + std::cos(a2 - 2.5f) * 5.f, q.y + std::sin(a2 - 2.5f) * 5.f}, Col(255, 120, 205));
    }
    FPt p = M(w.playerPos);
    float a = std::atan2(w.aimDir.y, w.aimDir.x);
    FPt tip{p.x + std::cos(a) * 7.f, p.y + std::sin(a) * 7.f};
    FPt l{p.x + std::cos(a + 2.5f) * 5.f, p.y + std::sin(a + 2.5f) * 5.f}, r{p.x + std::cos(a - 2.5f) * 5.f, p.y + std::sin(a - 2.5f) * 5.f};
    g.tri(tip, l, r, w.coop ? Col(110, 210, 255) : Col(120, 255, 160));
}
