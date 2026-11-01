#include "Renderer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

Col zombieColor(ZombieKind k) {
    switch (k) {
    case ZombieKind::Walker: return Col(110, 130, 80);
    case ZombieKind::Runner: return Col(205, 150, 60);
    case ZombieKind::Tank: return Col(140, 70, 130);
    case ZombieKind::Spitter: return Col(150, 200, 60);
    case ZombieKind::Boss: return Col(170, 40, 40);
    }
    return Col::White;
}

}  // namespace

SVec cameraCenter(const Game& w) {
    return SVec(std::clamp(w.playerPos.x, VIEW_W / 2.f, WORLD_W - VIEW_W / 2.f),
                std::clamp(w.playerPos.y, VIEW_H / 2.f, WORLD_H - VIEW_H / 2.f));
}

// ====================================================================== world
void drawWorld(Gfx& g, const Game& w, bool debugPaths) {
    SVec c = cameraCenter(w);
    g.beginWorld(c);

    // Only draw the tiles that are inside the camera.
    int x0 = std::max(0, (int)((c.x - VIEW_W / 2) / TILE)), x1 = std::min(COLS - 1, (int)((c.x + VIEW_W / 2) / TILE));
    int y0 = std::max(0, (int)((c.y - VIEW_H / 2) / TILE)), y1 = std::min(ROWS - 1, (int)((c.y + VIEW_H / 2) / TILE));
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            TileType t = w.graph.tile({x, y});
            float px = (float)(x * TILE), py = (float)(y * TILE);
            Col col;
            if (t == TileType::Road) { int v = 56 + ((x + y) & 1) * 4; col = Col(v, v + 2, v + 8); }
            else if (t == TileType::Rubble) col = Col(86, 78, 68);
            else if (t == TileType::Barricade) col = Col(150, 95, 40);
            else {
                bool interior = w.graph.isBuilding({x + 1, y}) && w.graph.isBuilding({x - 1, y}) &&
                                w.graph.isBuilding({x, y + 1}) && w.graph.isBuilding({x, y - 1});
                col = interior ? Col(74, 62, 52) : Col(104, 104, 112);
            }
            g.rect(px, py, (float)TILE, (float)TILE, col);
            if (t == TileType::Barricade) {
                g.rect(px + 5, py + 5, TILE - 10.f, TILE - 10.f, Col(110, 70, 30));
                float dmg = 1.f - w.barricadeFraction({x, y});            // darker = more damaged
                g.rect(px, py, (float)TILE, (float)TILE, Col(0, 0, 0, alpha8(dmg * 170.f)));
            }
            if (t == TileType::Rubble) {
                g.rect(px + 6, py + 8, 8, 6, Col(60, 54, 46));
                g.rect(px + 18, py + 16, 9, 7, Col(112, 102, 90));
            }
        }

    // mission goal area: green when unlocked, orange while locked
    const MissionDef& md = w.mission.def();
    bool unlocked = w.mission.goalUnlocked();
    Vec2 gp = w.mission.goalPos();
    g.rect(gp.x - 32.f, gp.y - 32.f, 64.f, 64.f, unlocked ? Col(40, 200, 90, 90) : Col(200, 80, 40, 70),
           unlocked ? Col(60, 255, 120) : Col(230, 110, 50), 2.f);
    g.text(unlocked ? std::string(md.zoneName) : std::string(md.zoneName) + " (LOCKED)", gp.x, gp.y - 52.f, 14,
           unlocked ? Col(60, 255, 120) : Col(240, 130, 70), 1);

    // route (yellow dots)
    for (size_t i = 0; i < w.route.size(); i += 2) g.circle(sv(tileCenter(w.route[i])), 3.f, Col(240, 210, 60, 170));

    // debug: every zombie's current path
    if (debugPaths)
        for (const Zombie& z : w.zombies) {
            if (z.path.size() < 2) continue;
            sf::VertexArray va(sf::PrimitiveType::LineStrip);
            va.append(sf::Vertex{sv(z.pos), Col(255, 80, 80)});
            for (size_t i = 1; i < z.path.size(); ++i) va.append(sf::Vertex{sv(tileCenter(z.path[i])), Col(255, 80, 80, 140)});
            g.window.draw(va);
        }

    SVec pp = sv(w.playerPos);

    // flashlight cone
    if (w.flashlight) {
        float ang = std::atan2(w.aimDir.y, w.aimDir.x);
        sf::ConvexShape cone(3);
        cone.setPoint(0, pp);
        cone.setPoint(1, pp + SVec(std::cos(ang - 0.4f), std::sin(ang - 0.4f)) * 260.f);
        cone.setPoint(2, pp + SVec(std::cos(ang + 0.4f), std::sin(ang + 0.4f)) * 260.f);
        cone.setFillColor(Col(255, 240, 150, 45));
        g.window.draw(cone);
    }

    // pickups
    for (const Pickup& k : w.pickups) {
        float x = k.pos.x, y = k.pos.y;
        if (k.kind == PickupKind::Health) {
            g.rect(x - 8, y - 8, 16, 16, Col(240, 240, 240));
            g.rect(x - 6, y - 2, 12, 4, Col(210, 40, 40));
            g.rect(x - 2, y - 6, 4, 12, Col(210, 40, 40));
        } else if (k.kind == PickupKind::Ammo) {
            g.rect(x - 8, y - 8, 16, 16, Col(90, 110, 60), Col(230, 200, 60), 2.f);
        } else if (k.kind == PickupKind::Grenade) {
            g.circle(SVec(x, y), 8.f, Col(60, 90, 50), Col(20, 40, 20), 2.f);
            g.rect(x - 2, y - 11, 4, 5, Col(180, 180, 180));
        } else {                                                    // fuel can
            g.rect(x - 7, y - 9, 14, 18, Col(200, 40, 40), Col(90, 10, 10), 2.f);
            g.rect(x - 3, y - 13, 6, 5, Col(240, 200, 60));
            g.text("F", x, y - 8.f, 12, Col::White, 1);
        }
    }

    // grenades (with blast radius marker) and explosions
    for (const Grenade& gr : w.grenades) {
        g.circle(sv(gr.target), 100.f, Col::Transparent, Col(255, 120, 40, 70), 1.f);
        g.circle(sv(gr.pos), 5.f, Col(60, 90, 50), Col(220, 60, 40), 2.f);
    }
    for (const Explosion& e : w.explosions) {
        float k = e.t / 0.45f;
        g.circle(sv(e.pos), 100.f * k, Col(255, 160, 40, alpha8(120.f * (1.f - k))), Col(255, 220, 120, alpha8(255.f * (1.f - k))), 3.f);
    }

    // zombies: red ring, coloured body, health bar when hurt, "!" when they heard a noise
    for (const Zombie& z : w.zombies) {
        if (z.kind == ZombieKind::Boss && z.charging > 0.f) g.circle(sv(z.pos), z.radius + 10.f, Col(255, 140, 40, 70), Col(255, 160, 60), 2.f);
        g.circle(sv(z.pos), z.radius + 4.f, Col::Transparent, Col(200, 40, 40, 200), 2.f);
        g.circle(sv(z.pos), z.radius, zombieColor(z.kind));
        if (z.kind == ZombieKind::Spitter) g.circle(sv(z.pos), 4.f, Col(220, 255, 100));
        if (z.hp < z.maxHp) {
            g.rect(z.pos.x - z.radius, z.pos.y - z.radius - 9.f, z.radius * 2.f, 4.f, Col(60, 0, 0));
            g.rect(z.pos.x - z.radius, z.pos.y - z.radius - 9.f, z.radius * 2.f * std::max(0.f, z.hp / z.maxHp), 4.f, Col(80, 220, 90));
        }
        if (z.alert > 0.f) g.text("!", z.pos.x, z.pos.y - z.radius - 24.f, 16, Col(255, 220, 60), 1);
    }

    for (const Acid& a : w.acids) g.circle(sv(a.pos), 5.f, Col(150, 230, 60, 220), Col(60, 120, 20), 1.f);
    for (const Bullet& b : w.bullets) g.circle(sv(b.pos), 2.5f, Col(255, 230, 120));

    // player + gun barrel
    g.circle(pp, PLAYER_RADIUS, Col(70, 190, 110), Col(20, 60, 30), 2.f);
    sf::RectangleShape gun(SVec(20.f, 4.f));
    gun.setOrigin(SVec(0.f, 2.f));
    gun.setPosition(pp);
    gun.setRotation(sf::degrees(std::atan2(w.aimDir.y, w.aimDir.x) * 180.f / PI));
    gun.setFillColor(Col(30, 30, 30));
    g.window.draw(gun);
}

// ====================================================================== minimap
namespace {
constexpr float MX = VIEW_W - 190.f, MY = 26.f, MW = 180.f, MH = 120.f, MS = 3.f;
}

void Minimap::rebuild(const Graph& graph) {
    va_.clear();
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            TileType t = graph.tile({x, y});
            if (t == TileType::Building) continue;
            Col c = t == TileType::Road ? Col(210, 210, 215) : t == TileType::Rubble ? Col(150, 130, 100) : Col(230, 70, 50);
            SVec a(MX + x * MS, MY + y * MS), b(a.x + MS, a.y), cc(a.x + MS, a.y + MS), d(a.x, a.y + MS);
            for (SVec v : {a, b, cc, a, cc, d}) va_.append(sf::Vertex{v, c});
        }
    builtFor_ = graph.version();
}

void Minimap::draw(Gfx& g, const Game& w) {
    if (builtFor_ != w.graph.version()) rebuild(w.graph);
    g.text("MINIMAP", MX + MW / 2.f, 6.f, 13, Col::White, 1);
    g.rect(MX, MY, MW, MH, Col(0, 0, 0, 220), Col::White, 1.f);
    g.window.draw(va_);
    auto toMini = [](Vec2 p) { return SVec(MX + p.x / WORLD_W * MW, MY + p.y / WORLD_H * MH); };
    for (const Zombie& z : w.zombies) {
        SVec m = toMini(z.pos);
        float s = z.kind == ZombieKind::Boss ? 5.f : 3.f;
        g.rect(m.x - s / 2.f, m.y - s / 2.f, s, s, Col(255, 60, 60));
    }
    for (const Pickup& p : w.pickups)
        if (p.kind == PickupKind::Fuel) { SVec m = toMini(p.pos); g.rect(m.x - 2, m.y - 2, 4, 4, Col(255, 150, 40)); }
    SVec s = toMini(w.mission.goalPos());
    g.rect(s.x - 3, s.y - 3, 6, 6, w.mission.goalUnlocked() ? Col(240, 210, 60) : Col(150, 120, 60));
    SVec p = toMini(w.playerPos);
    g.rect(p.x - 2, p.y - 2, 5, 5, Col(80, 255, 120));
}

// ====================================================================== HUD
void drawHud(Gfx& g, const Game& w, Minimap& minimap) {
    g.beginUi();

    // red flash when the player takes damage
    if (w.damageFlash > 0.02f) g.rect(0, 0, VIEW_W, UI_H, Col(200, 20, 20, alpha8(w.damageFlash * 110.f)));

    // --- score: big, in the top-left corner so it is easy to read
    g.panel(10, 10, 230, 62);
    g.text("SCORE", 125, 12, 13, Col(160, 190, 200), 1);
    g.text(std::to_string(w.score), 125, 28, 38, Col(120, 230, 255), 1);

    // --- health / ammo / stamina: compact bars in the bottom-left corner (keeps the player visible)
    {
        float px = 10.f, py = VIEW_H - 84.f, pw = 300.f;
        g.panel(px, py, pw, 74);
        float hp = std::max(0.f, w.hp);
        int maxAmmo = w.weapon == 0 ? 60 : 30;
        int ammo = w.inventory.count(w.weapon == 0 ? "Rifle" : "Pistol");
        const float bx = px + 80.f, bw = 150.f;
        auto bar = [&](float y, const char* label, Col labelCol, float frac, Col bg, Col fg, const std::string& val) {
            g.text(label, px + 8.f, y, 12, labelCol);
            g.rect(bx, y + 1.f, bw, 12, bg);
            g.rect(bx, y + 1.f, bw * std::max(0.f, std::min(1.f, frac)), 12, fg);
            g.text(val, px + pw - 8.f, y, 12, Col::White, 2);
        };
        bar(py + 6.f,  "HEALTH",  Col::White, hp / 100.f, Col(60, 20, 20), Col(190, 40, 40), std::to_string((int)hp) + "/100");
        bar(py + 31.f, "AMMO",    Col::White, ammo / (float)maxAmmo, Col(60, 50, 15), Col(220, 170, 30),
            std::to_string(ammo) + "/" + std::to_string(maxAmmo));
        bar(py + 54.f - 0.f, w.exhausted ? "TIRED" : "STAMINA", w.exhausted ? Col(240, 120, 90) : Col::White, w.stamina / 100.f,
            Col(15, 40, 60), w.exhausted ? Col(200, 90, 70) : Col(70, 160, 230), std::to_string((int)w.stamina) + "%");
    }

    minimap.draw(g, w);

    // --- warnings, banner, combo
    if (w.threatDistance >= 0.f && w.threatDistance < 150.f)
        g.text(std::string("DANGER: ") + kindName(w.threatKind), VIEW_W / 2.f, 14.f, 26, Col(240, 60, 60), 1);
    if (w.combo > 1) {
        g.text("COMBO x" + std::to_string(w.comboMultiplier()) + "  (" + std::to_string(w.combo) + " kills)", VIEW_W / 2.f, 50.f, 20, Col(255, 170, 50), 1);
        g.rect(VIEW_W / 2.f - 70.f, 76.f, 140.f, 5.f, Col(60, 40, 10));
        g.rect(VIEW_W / 2.f - 70.f, 76.f, 140.f * std::min(1.f, w.comboTimer / 3.f), 5.f, Col(255, 170, 50));
    }
    if (w.bannerTimer > 0.f)
        g.text(w.banner, VIEW_W / 2.f, 110.f, 36, Col(255, 220, 100, alpha8(std::min(1.f, w.bannerTimer / 0.6f) * 255.f)), 1);

    // --- hotbar
    const char* names[5] = {"Rifle", "Pistol", "Medkit", "Grenade", "Torch"};
    float x0 = (VIEW_W - 314.f) / 2.f, y0 = VIEW_H - 76.f;
    for (int i = 0; i < 5; ++i) {
        bool sel = (i == w.weapon && i < 2) || (i == 4 && w.flashlight);
        g.rect(x0 + i * 64.f, y0, 58, 62, Col(14, 18, 20, 230), sel ? Col(230, 200, 60) : Col(80, 90, 96), sel ? 2.f : 1.f);
        g.text(std::to_string(i + 1), x0 + i * 64.f + 4.f, y0 + 2.f, 12, Col(180, 180, 180));
        g.text(names[i], x0 + i * 64.f + 29.f, y0 + 20.f, 12, Col::White, 1);
        std::string cnt = i == 4 ? (w.flashlight ? "ON" : "OFF") : std::to_string(w.inventory.count(names[i]));
        g.text(cnt, x0 + i * 64.f + 29.f, y0 + 38.f, 16, Col(230, 200, 60), 1);
    }
}

// ====================================================================== sidebar
void drawSidebar(Gfx& g, const Game& w, const Leaderboard& board, bool paused) {
    g.beginUi();
    const float X = VIEW_W;
    g.rect(X, 0, 320, UI_H, Col(8, 10, 12));

    g.panel(X + 8, 8, 304, 70, "MAIN MENU");
    const char* labels[5] = {"PAUSE", "SAVE", "LOAD", "NEW", "MENU"};
    for (int i = 0; i < 5; ++i) g.button(sideButton(i), (i == 0 && paused) ? "RESUME" : labels[i], false, 11);

    // inventory: walks the LINKED LIST
    g.panel(X + 8, 86, 304, 100, "INVENTORY (linked list)");
    int i = 0;
    for (const ItemNode* n = w.inventory.head(); n; n = n->next, ++i) {
        float cx = X + 16.f + i * 48.f;
        g.rect(cx, 112, 44, 66, Col(24, 28, 32), Col(80, 90, 96), 1.f);
        g.text(n->name, cx + 22.f, 116.f, 10, Col(190, 190, 190), 1);
        std::string cnt = std::strcmp(n->name, "Torch") == 0 ? (w.flashlight ? "ON" : "OFF") : std::to_string(n->count);
        g.text(cnt, cx + 22.f, 142.f, 15, Col::White, 1);
    }

    // leaderboard: reads the BINARY SEARCH TREE
    g.panel(X + 8, 194, 304, 186, "LEADERBOARD (BST)");
    g.text("RANK  PLAYER", X + 16, 218, 11, Col(160, 160, 160));
    g.text("SCORE", X + 236, 218, 11, Col(160, 160, 160), 2);
    g.text("TIME", X + 300, 218, 11, Col(160, 160, 160), 2);
    int rank = 1;
    for (const ScoreRow& r : board.top(5)) {
        float y = 238.f + (rank - 1) * 22.f;
        Col c = r.name == "Player" ? Col(120, 230, 140) : Col::White;
        g.text(std::to_string(rank), X + 24, y, 13, c);
        g.text(r.name, X + 64, y, 13, c);
        g.text(std::to_string(r.score), X + 236, y, 13, c, 2);
        g.text(mmss(r.secs), X + 300, y, 13, c, 2);
        ++rank;
    }
    g.rect(X + 12, 354, 296, 22, Col(30, 70, 40));
    g.text(std::string("Player (live) ") + diffName(w.difficulty), X + 24, 357, 13, Col(160, 255, 170));
    g.text(std::to_string(w.score), X + 236, 357, 13, Col(160, 255, 170), 2);
    g.text(mmss((int)w.gameTime), X + 300, 357, 13, Col(160, 255, 170), 2);

    // live data-structure statistics
    g.panel(X + 8, 388, 304, 324, "DATA STRUCTURES (live)");
    float y = 412.f;
    auto line = [&](const std::string& s, Col c = Col(210, 210, 215)) { g.text(s, X + 18, y, 13, c); y += 21.f; };
    char buf[110];
    std::snprintf(buf, sizeof buf, "Graph: %d nodes, %d edges (adjacency list)", w.graph.nodeCount(), w.graph.edgeCount());
    line(buf);
    line(std::string("Zombie AI: ") + (w.useDijkstra ? "DIJKSTRA (weighted)" : "BFS (fewest steps)") + "  [B]", Col(240, 210, 90));
    std::snprintf(buf, sizeof buf, "BFS %.0f us (%ld) | DIJ %.0f us (%ld)", w.bfsStat.avgUs, w.bfsStat.calls, w.dijStat.avgUs, w.dijStat.calls);
    line(buf, Col(120, 230, 255));
    line("Route to goal: " + (w.route.empty() ? std::string("none") : std::to_string(w.route.size() - 1) + " tiles"),
         w.route.empty() ? Col(170, 170, 170) : Col(210, 210, 215));
    line("Queue (waves): " + std::to_string(w.waves.size()) + " left");
    line("Priority Queue: " + (w.threatDistance < 0.f ? std::string("no threat") :
                               std::string(kindName(w.threatKind)) + " at " + std::to_string((int)w.threatDistance) + " px"));
    line("Linked List: " + std::to_string(w.inventory.size()) + " items");
    line("Stack (undo): " + std::to_string(w.undo.size()) + " | " + w.undo.describeTop());
    line("BST: " + std::to_string(board.size()) + " scores, height " + std::to_string(board.height()));
    line("Zombies  W:" + std::to_string(w.zombieCount(ZombieKind::Walker)) + " R:" + std::to_string(w.zombieCount(ZombieKind::Runner)) +
         " T:" + std::to_string(w.zombieCount(ZombieKind::Tank)) + " S:" + std::to_string(w.zombieCount(ZombieKind::Spitter)) +
         " B:" + std::to_string(w.zombieCount(ZombieKind::Boss)));
    y += 2.f;
    const char* help[4] = {"WASD move | SHIFT sprint | LMB shoot", "RMB barricade | E grenade | U undo",
                           "1/2 weapon | 3 medkit | 5 torch", "B algorithm | TAB paths | F5/F9 | F11"};
    for (const char* h : help) { g.text(h, X + 18, y, 12, Col(140, 140, 140)); y += 16.f; }
}
