#include "App.h"
#include <algorithm>
#include <cmath>
#include <optional>
#include "GameSave.h"

namespace {
const char* const SAVE_FILE = "savegame.txt";
const char* const BOARD_FILE = "leaderboard.txt";


constexpr float 
bool gameInProgress = false;     

sf::Clock survivalClock;
float survivalOffset = 0.f;      
bool survivalActive = false;     

bool isSurvivalLevel(const Game& w) { return w.mission.def().type == MissionType::SurviveThenReach; }

void resetSurvivalTimer(const Game& w) {
    survivalActive = isSurvivalLevel(w);
    survivalOffset = survivalActive ? w.missionTime : 0.f;   
    survivalClock.reset();                                   
}

float survivalSeconds() { return survivalOffset + survivalClock.getElapsedTime().asSeconds(); }

void ensureFullscreen(Gfx& g) {
    if (!g.isFullscreen()) g.createWindow(true);
}

SVec fullCameraCenter(const Game& w) {
    return SVec(std::clamp(w.playerPos.x, PLAY_W / 2.f, WORLD_W - PLAY_W / 2.f),
                std::clamp(w.playerPos.y, PLAY_H / 2.f, WORLD_H - PLAY_H / 2.f));
}


void applyFullView(Gfx& g, const Game& w) {
    g.worldView.setSize(SVec(PLAY_W, PLAY_H));
    g.worldView.setViewport(g.uiView.getViewport());
    g.worldView.setCenter(fullCameraCenter(w));
}

Col zombieColorFull(ZombieKind k) {
    switch (k) {
    case ZombieKind::Walker: return Col(110, 130, 80);
    case ZombieKind::Runner: return Col(205, 150, 60);
    case ZombieKind::Tank: return Col(140, 70, 130);
    case ZombieKind::Spitter: return Col(150, 200, 60);
    case ZombieKind::Boss: return Col(170, 40, 40);
    }
    return Col::White;
}
void drawWorldFull(Gfx& g, const Game& w, bool debugPaths) {
    applyFullView(g, w);                  
    SVec c = g.worldView.getCenter();
    g.window.setView(g.worldView);

    
    int x0 = std::max(0, (int)((c.x - PLAY_W / 2) / TILE)), x1 = std::min(COLS - 1, (int)((c.x + PLAY_W / 2) / TILE));
    int y0 = std::max(0, (int)((c.y - PLAY_H / 2) / TILE)), y1 = std::min(ROWS - 1, (int)((c.y + PLAY_H / 2) / TILE));
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

    const MissionDef& md = w.mission.def();
    bool unlocked = w.mission.goalUnlocked();
    Vec2 gp = w.mission.goalPos();
    g.rect(gp.x - 32.f, gp.y - 32.f, 64.f, 64.f, unlocked ? Col(40, 200, 90, 90) : Col(200, 80, 40, 70),
           unlocked ? Col(60, 255, 120) : Col(230, 110, 50), 2.f);
    g.text(unlocked ? std::string(md.zoneName) : std::string(md.zoneName) + " (LOCKED)", gp.x, gp.y - 52.f, 14,
           unlocked ? Col(60, 255, 120) : Col(240, 130, 70), 1);


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


    if (w.flashlight) {
        float ang = std::atan2(w.aimDir.y, w.aimDir.x);
        sf::ConvexShape cone(3);
        cone.setPoint(0, pp);
        cone.setPoint(1, pp + SVec(std::cos(ang - 0.4f), std::sin(ang - 0.4f)) * 260.f);
        cone.setPoint(2, pp + SVec(std::cos(ang + 0.4f), std::sin(ang + 0.4f)) * 260.f);
        cone.setFillColor(Col(255, 240, 150, 45));
        g.window.draw(cone);
    }

  
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

 
    for (const Grenade& gr : w.grenades) {
        g.circle(sv(gr.target), 100.f, Col::Transparent, Col(255, 120, 40, 70), 1.f);
        g.circle(sv(gr.pos), 5.f, Col(60, 90, 50), Col(220, 60, 40), 2.f);
    }
    for (const Explosion& e : w.explosions) {
        float k = e.t / 0.45f;
        g.circle(sv(e.pos), 100.f * k, Col(255, 160, 40, alpha8(120.f * (1.f - k))), Col(255, 220, 120, alpha8(255.f * (1.f - k))), 3.f);
    }

    
    for (const Zombie& z : w.zombies) {
        if (z.kind == ZombieKind::Boss && z.charging > 0.f) g.circle(sv(z.pos), z.radius + 10.f, Col(255, 140, 40, 70), Col(255, 160, 60), 2.f);
        g.circle(sv(z.pos), z.radius + 4.f, Col::Transparent, Col(200, 40, 40, 200), 2.f);
        g.circle(sv(z.pos), z.radius, zombieColorFull(z.kind));
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

void iconHeart(Gfx& g, float cx, float cy) {
    Col red(220, 50, 60);
    g.circle(SVec(cx - 4.f, cy - 3.f), 5.f, red);
    g.circle(SVec(cx + 4.f, cy - 3.f), 5.f, red);
    sf::ConvexShape t(3);
    t.setPoint(0, SVec(cx - 9.f, cy - 1.f));
    t.setPoint(1, SVec(cx + 9.f, cy - 1.f));
    t.setPoint(2, SVec(cx, cy + 9.f));
    t.setFillColor(red);
    g.window.draw(t);
}
void iconGun(Gfx& g, float cx, float cy) {
    g.rect(cx - 10.f, cy - 4.f, 20.f, 5.f, Col(190, 195, 200));
    g.rect(cx - 6.f, cy + 1.f, 4.f, 8.f, Col(150, 155, 160));
}
void iconBullet(Gfx& g, float cx, float cy) {
    g.rect(cx - 3.f, cy - 3.f, 6.f, 11.f, Col(220, 170, 30));
    g.circle(SVec(cx, cy - 3.f), 3.f, Col(240, 200, 90));
}
void iconMedkit(Gfx& g, float cx, float cy) {
    g.rect(cx - 8.f, cy - 7.f, 16.f, 14.f, Col(240, 240, 240));
    g.rect(cx - 5.f, cy - 1.5f, 10.f, 3.f, Col(210, 40, 40));
    g.rect(cx - 1.5f, cy - 5.f, 3.f, 10.f, Col(210, 40, 40));
}
void iconGrenade(Gfx& g, float cx, float cy) {
    g.circle(SVec(cx, cy + 1.f), 6.f, Col(70, 110, 60), Col(20, 40, 20), 1.f);
    g.rect(cx - 2.f, cy - 9.f, 4.f, 4.f, Col(180, 180, 180));
}
void iconFlag(Gfx& g, float cx, float cy) {
    g.rect(cx - 6.f, cy - 9.f, 2.f, 18.f, Col(200, 200, 200));
    sf::ConvexShape t(3);
    t.setPoint(0, SVec(cx - 4.f, cy - 9.f));
    t.setPoint(1, SVec(cx + 8.f, cy - 5.f));
    t.setPoint(2, SVec(cx - 4.f, cy - 1.f));
    t.setFillColor(Col(230, 170, 40));
    g.window.draw(t);
}

void drawBottomHud(Gfx& g, const Game& w) {
    const float W = 1120.f, H = 50.f, X = (UI_W - W) / 2.f, Y = UI_H - H - 8.f;
    g.rect(X, Y, W, H, Col(10, 14, 18, 200), Col(70, 80, 88), 1.f);
    const Col labelCol(150, 160, 170);
    const float ly = Y + 6.f, vy = Y + 20.f, iy = Y + H / 2.f;
    float x = X;
    auto divider = [&](float width) { x += width; if (x < X + W - 1.f) g.rect(x, Y + 8.f, 1.f, H - 16.f, Col(70, 80, 88)); };

    
    float hp = std::max(0.f, w.hp);
    iconHeart(g, x + 18.f, iy);
    g.text("HEALTH", x + 34.f, ly, 11, labelCol);
    g.text(std::to_string((int)std::ceil(hp)), x + 34.f, vy, 20, hp < 30.f ? Col(255, 90, 90) : Col::White);
    g.rect(x + 90.f, Y + 29.f, 96.f, 8.f, Col(60, 20, 20));
    g.rect(x + 90.f, Y + 29.f, 96.f * std::min(1.f, hp / 100.f), 8.f, Col(190, 40, 40));
    divider(200.f);

    
    const bool rifle = w.weapon == 0;
    iconGun(g, x + 20.f, iy);
    g.text("WEAPON", x + 38.f, ly, 11, labelCol);
    g.text(rifle ? "RIFLE" : "PISTOL", x + 38.f, vy, 20, Col(230, 200, 60));
    divider(150.f);

    int ammo = w.inventory.count(rifle ? "Rifle" : "Pistol");
    iconBullet(g, x + 18.f, iy);
    g.text(rifle ? "RIFLE AMMO" : "PISTOL AMMO", x + 34.f, ly, 11, labelCol);
    g.text(std::to_string(ammo), x + 34.f, vy, 20, ammo == 0 ? Col(255, 90, 90) : Col::White);
    divider(150.f);


    iconMedkit(g, x + 18.f, iy);
    g.text("MEDKITS", x + 34.f, ly, 11, labelCol);
    g.text(std::to_string(w.inventory.count("Medkit")), x + 34.f, vy, 20, Col::White);
    divider(130.f);

 
    iconGrenade(g, x + 18.f, iy);
    g.text("GRENADES", x + 34.f, ly, 11, labelCol);
    g.text(std::to_string(w.inventory.count("Grenade")), x + 34.f, vy, 20, Col::White);
    divider(130.f);


    g.text("SCORE", x + 14.f, ly, 11, labelCol);
    g.text(std::to_string(w.score), x + 14.f, Y + 16.f, 26, Col(120, 230, 255));
    divider(200.f);

    iconFlag(g, x + 18.f, iy);
    g.text(std::string("LEVEL - ") + w.mission.def().title, x + 34.f, ly, 11, labelCol);
    g.text(std::to_string(w.mission.index() + 1) + " / " + std::to_string(MissionManager::count()), x + 34.f, vy, 20, Col::White);
}

void drawSurvivalTimer(Gfx& g, const Game& w) {
    if (!survivalActive || !isSurvivalLevel(w)) return;
    int secs = (int)survivalSeconds();
    g.rect(14.f, 14.f, 320.f, 62.f, Col(10, 14, 18, 200), Col(230, 170, 40), 1.f);
    g.text("Survival Time: " + std::to_string(secs) + " sec", 26.f, 20.f, 22, Col(255, 220, 100));
    float left = w.mission.surviveLeft();
    std::string sub = left > 0.f
        ? "Goal: survive " + std::to_string((int)w.mission.def().surviveSeconds) + " sec  (" + std::to_string((int)std::ceil(left)) + " sec left)"
        : std::string("Survived! Now reach the ") + w.mission.def().zoneName;
    g.text(sub, 26.f, 50.f, 14, left > 0.f ? Col(210, 210, 210) : Col(120, 230, 140));
}

void drawGameplayUi(Gfx& g, const Game& w, Minimap& minimap) {
    g.beginUi();
    if (w.damageFlash > 0.02f) g.rect(0, 0, UI_W, UI_H, Col(200, 20, 20, alpha8(w.damageFlash * 110.f)));

    sf::View v = g.uiView;
    v.move(SVec(-(UI_W - VIEW_W), 0.f));
    g.window.setView(v);
    minimap.draw(g, w);
    g.beginUi();

    const float cx = UI_W / 2.f;
    if (w.threatDistance >= 0.f && w.threatDistance < 150.f)
        g.text(std::string("DANGER: ") + kindName(w.threatKind), cx, 14.f, 26, Col(240, 60, 60), 1);
    if (w.combo > 1) {
        g.text("COMBO x" + std::to_string(w.comboMultiplier()) + "  (" + std::to_string(w.combo) + " kills)", cx, 50.f, 20, Col(255, 170, 50), 1);
        g.rect(cx - 70.f, 76.f, 140.f, 5.f, Col(60, 40, 10));
        g.rect(cx - 70.f, 76.f, 140.f * std::min(1.f, w.comboTimer / 3.f), 5.f, Col(255, 170, 50));
    }
    if (w.bannerTimer > 0.f)
        g.text(w.banner, cx, 110.f, 36, Col(255, 220, 100, alpha8(std::min(1.f, w.bannerTimer / 0.6f) * 255.f)), 1);

    drawSurvivalTimer(g, w);
    drawBottomHud(g, w);
}


void drawOverlayFull(Gfx& g, Screen s, const Game& w) {
    if (s == Screen::Playing) return;
    g.beginUi();
    const float cx = UI_W / 2.f;
    g.rect(0, 0, UI_W, UI_H, Col(0, 0, 0, 170));
    std::string title = s == Screen::Paused ? "PAUSED" : s == Screen::MissionClear ? "MISSION COMPLETE"
                      : s == Screen::Won ? "CAMPAIGN COMPLETE" : "GAME OVER";
    Col tc = (s == Screen::Won || s == Screen::MissionClear) ? Col(80, 230, 120) : (s == Screen::Lost ? Col(230, 70, 70) : Col::White);
    g.text(title, cx, 220.f, 52, tc, 1);

    if (s == Screen::Paused) {
        g.text("ESC or P = resume   |   F5 = save   |   F9 = load   |   M = main menu", cx, 300.f, 22, Col(210, 210, 210), 1);
        g.text("LEVEL " + std::to_string(w.mission.index() + 1) + ": " + w.mission.def().title + "  -  " + w.mission.def().brief,
               cx, 345.f, 18, Col(230, 170, 40), 1);
        g.text(w.mission.progressText(), cx, 375.f, 18, Col(210, 210, 210), 1);
        return;
    }
    g.text("Score: " + std::to_string(w.score) + "   Kills: " + std::to_string(w.kills) + "   Time: " + mmss((int)w.gameTime),
           cx, 300.f, 22, Col::White, 1);
    int done = 0;
    for (int i = 0; i < 3; ++i) if (w.objectivesDone & (1u << i)) ++done;
    g.text("Bonus objectives: " + std::to_string(done) + "/3   Difficulty: " + diffName(w.difficulty), cx, 335.f, 18, Col(210, 210, 210), 1);

    if (s == Screen::MissionClear) {
        const MissionDef& next = MissionManager::defOf(w.mission.index() + 1);
        g.text("NEXT: MISSION " + std::to_string(w.mission.index() + 2) + " - " + next.title, cx, 380.f, 22, Col(230, 170, 40), 1);
        g.text(next.brief, cx, 412.f, 16, Col(200, 200, 200), 1);
        g.text("ENTER = next mission   |   ESC = main menu", cx, 460.f, 20, Col(210, 210, 210), 1);
    } else {
        g.text("ENTER = play again   |   ESC = main menu", cx, 390.f, 20, Col(210, 210, 210), 1);
    }
}

void drawToastFull(Gfx& g, const std::string& message) {
    g.beginUi();
    g.rect(UI_W / 2.f - 160.f, 86.f, 320.f, 30.f, Col(20, 24, 28, 230), Col(230, 170, 40), 1.f);
    g.text(message, UI_W / 2.f, 92.f, 16, Col::White, 1);
}
}  

App::App(bool fullscreen) {
    gfx_.createWindow(fullscreen);
    if (!board_.load(BOARD_FILE)) board_.seedSamples();
    game_.newGame(difficulty_);            
}

void App::run() {
    sf::Clock clock;
    while (gfx_.window.isOpen()) {
        float dt = std::min(clock.restart().asSeconds(), 0.05f);   
        handleEvents();
        if (screen_ == Screen::Playing) update(dt);

        bool tick = survivalActive && screen_ == Screen::Playing && game_.status == Status::Playing;
        if (tick && !survivalClock.isRunning()) survivalClock.start();
        if (!tick && survivalClock.isRunning()) survivalClock.stop();
        toastTimer_ -= dt;
        draw();
    }
}

void App::startNew() {
    game_.newGame(difficulty_);
    runRecorded_ = false;
    gameInProgress = true;
    resetSurvivalTimer(game_);
    ensureFullscreen(gfx_);
    screen_ = Screen::Playing;
}

void App::doSave() { toast(saveGameToFile(game_.toSave(), SAVE_FILE) ? "Game saved" : "Save failed"); }

bool App::doLoad() {
    SaveData d;
    if (!loadGameFromFile(d, SAVE_FILE)) { toast("No valid save file"); return false; }
    game_.fromSave(d);
    difficulty_ = d.difficulty;
    runRecorded_ = false;
    gameInProgress = true;
    resetSurvivalTimer(game_);       
    toast("Game loaded");
    return true;
}

void App::finishRun() {
    if (!runRecorded_) {
        board_.add("Player", game_.score, (int)game_.gameTime, (int)game_.difficulty);
        board_.save(BOARD_FILE);
        runRecorded_ = true;
    }
    gameInProgress = false;          
    screen_ = game_.status == Status::Won ? Screen::Won : Screen::Lost;
}

void App::handleEvents() {
    applyFullView(gfx_, game_);                         
    while (const std::optional ev = gfx_.window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) gfx_.window.close();
        if (ev->is<sf::Event::Resized>()) { gfx_.updateViewports(); applyFullView(gfx_, game_); }
        if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) onKey(k->code);
        if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
            SVec m = gfx_.toUi(mb->position);
            if (mb->button == sf::Mouse::Button::Left) onLeftClick(m);
            if (mb->button == sf::Mouse::Button::Right && screen_ == Screen::Playing) {   
                applyFullView(gfx_, game_);
                SVec wp = gfx_.toWorld(mb->position);
                game_.toggleBarricade(Vec2(wp.x, wp.y));
            }
        }
    }
}

void App::onKey(sf::Keyboard::Key key) {
    using K = sf::Keyboard::Key;
    if (key == K::F11) { gfx_.createWindow(!gfx_.isFullscreen()); return; }

    switch (screen_) {
    case Screen::Menu:
        if (key == K::Enter) startNew();
        break;
    case Screen::Leaderboard:
    case Screen::Help:
        if (key == K::Escape) screen_ = Screen::Menu;
        break;
    case Screen::MissionClear:
        if (key == K::Enter) { game_.nextMission(); resetSurvivalTimer(game_); ensureFullscreen(gfx_); screen_ = Screen::Playing; }
        if (key == K::Escape) screen_ = Screen::Menu;
        break;
    case Screen::Won:
    case Screen::Lost:
        if (key == K::Enter) startNew();
        if (key == K::Escape) screen_ = Screen::Menu;
        break;
    case Screen::Paused:
        if (key == K::Escape || key == K::P) screen_ = Screen::Playing;
        if (key == K::F5) doSave();
        if (key == K::F9) doLoad();
        if (key == K::M) screen_ = Screen::Menu;      
        break;
    case Screen::Playing:
        if (key == K::Escape || key == K::P) screen_ = Screen::Paused;
        if (key == K::Num1) game_.weapon = 0;
        if (key == K::Num2) game_.weapon = 1;
        if (key == K::Num3) game_.useMedkit();
        if (key == K::Num5 || key == K::F) game_.flashlight = !game_.flashlight;
        if (key == K::E) {
            SVec mw = gfx_.toWorld(sf::Mouse::getPosition(gfx_.window));
            if (!game_.throwGrenade(Vec2(mw.x, mw.y))) toast("No grenades left");
        }
        if (key == K::U) { if (!game_.undoBarricade()) toast("Nothing to undo"); }
        if (key == K::B) {
            game_.useDijkstra = !game_.useDijkstra;
            game_.invalidatePaths();
            toast(game_.useDijkstra ? "Zombie AI: Dijkstra" : "Zombie AI: BFS");
        }
        if (key == K::Tab) debugPaths_ = !debugPaths_;
        if (key == K::F5) doSave();
        if (key == K::F9) doLoad();
        break;
    }
}

void App::onLeftClick(SVec m) {
    if (screen_ == Screen::Menu) {
        if (menuButton(0).contains(m)) startNew();
        else if (menuButton(1).contains(m)) {
            if (gameInProgress) { ensureFullscreen(gfx_); screen_ = Screen::Paused; }   
            else if (doLoad()) { ensureFullscreen(gfx_); screen_ = Screen::Playing; } 
        }
        else if (menuButton(2).contains(m)) difficulty_ = (Difficulty)(((int)difficulty_ + 1) % 3);
        else if (menuButton(3).contains(m)) screen_ = Screen::Leaderboard;
        else if (menuButton(4).contains(m)) screen_ = Screen::Help;
        else if (menuButton(5).contains(m)) gfx_.createWindow(!gfx_.isFullscreen());
        else if (menuButton(6).contains(m)) gfx_.window.close();
    } else if (screen_ == Screen::Leaderboard || screen_ == Screen::Help) {
        if (backButton().contains(m)) screen_ = Screen::Menu;
    }
}

void App::update(float dt) {
    applyFullView(gfx_, game_);
    Input in;
    using K = sf::Keyboard::Key;
    auto down = [](K k) { return sf::Keyboard::isKeyPressed(k); };
    if (down(K::W) || down(K::Up)) in.move.y -= 1.f;
    if (down(K::S) || down(K::Down)) in.move.y += 1.f;
    if (down(K::A) || down(K::Left)) in.move.x -= 1.f;
    if (down(K::D) || down(K::Right)) in.move.x += 1.f;
    in.sprint = down(K::LShift) || down(K::RShift);

    SVec mw = gfx_.toWorld(sf::Mouse::getPosition(gfx_.window));
    Vec2 aim = Vec2(mw.x, mw.y) - game_.playerPos;
    in.aim = aim.length() > 1.f ? aim / aim.length() : game_.aimDir;
    in.fire = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);   

    game_.update(dt, in);

    if (game_.status == Status::MissionClear) screen_ = Screen::MissionClear;
    else if (game_.status != Status::Playing) finishRun();
}
wing
void App::draw() {
    gfx_.window.clear(Col(0, 0, 0));                 
    gfx_.beginUi();
    gfx_.rect(0, 0, UI_W, UI_H, Col(12, 14, 16));
    switch (screen_) {
    case Screen::Menu:
        drawMenu(gfx_, saveFileExists(SAVE_FILE) || gameInProgress, difficulty_);
        if (gameInProgress) gfx_.button(menuButton(1), "RESUME GAME");   
        break;
    case Screen::Leaderboard: drawLeaderboardScreen(gfx_, board_); break;
    case Screen::Help: drawHelp(gfx_); break;
    default:
        drawWorldFull(gfx_, game_, debugPaths_);     
        drawGameplayUi(gfx_, game_, minimap_);       
        drawOverlayFull(gfx_, screen_, game_);
        break;
    }
    if (toastTimer_ > 0.f) drawToastFull(gfx_, toastText_);
    gfx_.window.display();
}
