#include "Screens.h"
#include <algorithm>
#include <cmath>

namespace {
float keycap(Gfx& g, const std::string& k, float x, float cy, float fs = 12.f) {
    float w = g.textWidth(k, fs, Font::Title) + 14.f, h = fs + 12.f;
    g.roundRect(x, cy - h / 2 + 2, w, h, 5, Col(0, 0, 0, 120));
    g.roundRect(x, cy - h / 2, w, h, 5, Col(70, 78, 92), Col(46, 52, 64));
    g.roundOutline(x, cy - h / 2, w, h, 5, Col(255, 255, 255, 40));
    g.textMid(k, x + w / 2, cy, fs, C::White, Align::Center, Font::Title);
    return w;
}

std::string diffLabel(Difficulty d) { return diffName(d); }
Col diffColor(int d) { return d == 0 ? Col(110, 220, 140) : d == 1 ? Col(255, 196, 80) : Col(255, 96, 80); }
}  


namespace {
void flagIcon(Gfx& g, float x, float y) {                
    g.rect(x, y, 2, 18, Col(200, 200, 200));
    g.tri(FPt{x + 2, y}, FPt{x + 14, y + 4.5f}, FPt{x + 2, y + 9}, Col(255, 170, 30));
}
}  

void drawHud(Gfx& g, const Art& a, WorldRenderer& world, const Game& w, const Fx& fx) {
    g.ui();
    const float t = (float)g.time;

    g.image(a.vignette, 0, 0, VIEW_W, VIEW_H, Col(0, 0, 0, 190));
    float hurt = std::max(w.damageFlash, fx.hurt);
    if (w.hp < 30.f) hurt = std::max(hurt, 0.35f + 0.25f * std::pow(std::fabs(std::sin(t * 3.2f)), 6.f));
    if (hurt > 0.02f) g.image(a.vignette, 0, 0, VIEW_W, VIEW_H, Col(210, 16, 16, alpha8(hurt * 255.f)));
    const bool danger = w.threatDistance >= 0.f && w.threatDistance < 150.f;
    if (danger) g.image(a.vignette, 0, 0, VIEW_W, VIEW_H, Col(255, 40, 30, alpha8(70 + 50 * std::sin(t * 9.f))));

    if (w.bannerTimer > 0.f) {
        const float cx = VIEW_W / 2;
        float k = std::min(1.f, w.bannerTimer / 0.5f);
        float in = clamp01((3.f - w.bannerTimer) * 6.f);
        float y = 190 - (1.f - in) * 20.f;
        g.rectHGrad(cx - 420, y - 8, 420, 64, Col(0, 0, 0, 0), Col(0, 0, 0, alpha8(150 * k)));
        g.rectHGrad(cx, y - 8, 420, 64, Col(0, 0, 0, alpha8(150 * k)), Col(0, 0, 0, 0));
        g.rectHGrad(cx - 300, y - 8, 300, 2, Col(255, 176, 46, 0), C::Accent.alpha(k));
        g.rectHGrad(cx, y - 8, 300, 2, C::Accent.alpha(k), Col(255, 176, 46, 0));
        g.textShadow(w.banner, cx, y, 32, Col(255, 222, 140, alpha8(255 * k)), Align::Center, Font::Title, 3.f);
    }

    const float BY = VIEW_H;                                   
    const Col bg(26, 29, 36), lineCol(68, 72, 82), lab(150, 156, 166), val(236, 238, 242);
    const Col weaponCol(240, 200, 60), scoreCol(80, 200, 240);
    g.rect(0, BY, UI_W, HUD_H, bg);
    g.rect(0, BY, UI_W, 2, Col(56, 60, 70));
    g.rect(0, BY + 2, UI_W, 1, Col(0, 0, 0, 160));

    struct Cell { float x, w; };
    const Cell cells[8] = {{12, 200}, {212, 140}, {352, 130}, {482, 110}, {592, 115}, {707, 115}, {822, 135}, {957, 160}};
    for (int i = 1; i < 8; ++i) g.rect(cells[i].x, BY + 12, 1, 44, lineCol);
    const float LY = BY + 11, VY = BY + 25;
    auto labelAt = [&](const std::string& s2, float x) { g.text(s2, x, LY, 11, lab, Align::Left, Font::Plain); };
    auto valueAt = [&](const std::string& s2, float x, Col c) { return g.text(s2, x, VY, 22, c, Align::Left, Font::Plain); };

    if (w.coop) {
        float x = cells[0].x;
        labelAt("HEALTH", x + 8);
        const Col tcol[2] = {Col(90, 205, 255), Col(255, 110, 200)};
        for (int p = 0; p < 2; ++p) {
            float hp = p == 0 ? w.hp : w.p2.hp, st = p == 0 ? w.stamina : w.p2.stamina;
            bool down = p == 0 ? w.p1Down : w.p2Down;
            float yy = BY + 27 + p * 16.f;
            g.roundRect(x + 8, yy, 22, 13, 6, tcol[p]);
            g.textMid(p == 0 ? "P1" : "P2", x + 19, yy + 6.5f, 9, Col(10, 16, 28), Align::Center, Font::Title);
            if (down) { g.text("DOWN", x + 38, yy - 1, 12, Col(255, 80, 70), Align::Left, Font::Title); continue; }
            g.text(std::to_string((int)std::ceil(std::max(0.f, hp))), x + 38, yy - 2, 13, hp < 30.f ? Col(255, 90, 90) : val, Align::Left, Font::Plain);
            g.rect(x + 72, yy + 2, 116, 6, Col(60, 20, 24));
            g.rect(x + 72, yy + 2, 116 * clamp01(hp / 100.f), 6, Col(205, 34, 40));
            g.rect(x + 72, yy + 10, 116 * clamp01(st / 100.f), 2, Col(90, 170, 255));
        }
    } else {
        float x = cells[0].x;
        float hp = std::max(0.f, w.hp);
        g.sprite(a.icoHeart, x + 14, BY + 33, 24, 16, 0, Col(225, 40, 50));
        labelAt("HEALTH", x + 34);
        valueAt(std::to_string((int)std::ceil(hp)), x + 34, hp < 30.f ? Col(255, 90, 90) : val);
        g.rect(x + 92, BY + 33, 100, 8, Col(60, 20, 24));
        g.rect(x + 92, BY + 33, 100 * clamp01(hp / 100.f), 8, Col(205, 34, 40));
        g.rect(x + 92, BY + 45, 100, 3, Col(30, 40, 60));
        g.rect(x + 92, BY + 45, 100 * clamp01(w.stamina / 100.f), 3, w.exhausted ? Col(240, 120, 80) : Col(90, 170, 255));
    }
    
    if (w.coop) {
        float x = cells[1].x + 12;
        labelAt("WEAPONS", x);
        g.text(std::string("P1 ") + (w.weapon == 0 ? "RIFLE" : "PISTOL"), x, BY + 26, 13, weaponCol, Align::Left, Font::Plain);
        g.text(std::string("P2 ") + (w.p2.weapon == 0 ? "RIFLE" : "PISTOL"), x, BY + 42, 13, Col(255, 150, 215), Align::Left, Font::Plain);
    } else {
        float x = cells[1].x + 12;
        g.sprite(w.weapon == 0 ? a.icoRifle : a.icoPistol, x + 14, BY + 34, 30, 20, 0, Col(200, 204, 210));
        labelAt("WEAPON", x + 36);
        valueAt(w.weapon == 0 ? "RIFLE" : "PISTOL", x + 36, weaponCol);
    }
    
    int rifle = w.inventory.count("Rifle"), pistol = w.inventory.count("Pistol");
    if (w.coop) {
        float x = cells[2].x + 12;
        labelAt("TEAM AMMO", x);
        g.text("Rifle  " + std::to_string(rifle), x, BY + 26, 13, rifle ? val : Col(255, 80, 70), Align::Left, Font::Plain);
        g.text("Pistol " + std::to_string(pistol), x, BY + 42, 13, pistol ? val : Col(255, 80, 70), Align::Left, Font::Plain);
    } else {
        float x = cells[2].x + 12;
        int ammo = w.weapon == 0 ? rifle : pistol;
        g.roundRect(x + 2, BY + 27, 6, 14, 2, Col(230, 180, 50));
        labelAt(w.weapon == 0 ? "RIFLE AMMO" : "PISTOL AMMO", x + 20);
        valueAt(std::to_string(ammo), x + 20, ammo == 0 ? Col(255, 80, 70) : ammo < 10 ? Col(255, 150, 90) : val);
    }
    
    auto itemCell = [&](int ci, SDL_Texture* icon, const char* name, int count, const char* key) {
        float x = cells[ci].x + 12;
        g.sprite(icon, x + 9, BY + 34, 20, 20);
        labelAt(name, x + 26);
        float vw = valueAt(std::to_string(count), x + 26, count == 0 ? Col(255, 90, 90) : val);
        g.text(key, x + 26 + vw + 8, VY + 8, 10, C::Faint, Align::Left, Font::Plain);
    };
    itemCell(3, a.pickup[(int)PickupKind::Health], "MEDKITS", w.inventory.count("Medkit"), "[3]");
    itemCell(4, a.pickup[(int)PickupKind::Grenade], "GRENADES", w.inventory.count("Grenade"), "[E]");
    itemCell(5, a.barricade, "BARRIERS", w.inventory.count("Barrier"), "[RMB]");
    
    {
        float x = cells[6].x + 12;
        labelAt("SCORE", x);
        float vw = valueAt(std::to_string(w.score), x, scoreCol);
        if (w.comboMultiplier() > 1) {
            std::string c = "x" + std::to_string(w.comboMultiplier());
            float cw = g.textWidth(c, 12, Font::Title) + 12;
            g.roundRect(x + vw + 8, VY + 4, cw, 20, 10, Col(255, 150, 40));
            g.textMid(c, x + vw + 8 + cw / 2, VY + 14, 12, Col(40, 18, 0), Align::Center, Font::Title);
        }
    }
    
    {
        float x = cells[7].x + 12;
        const MissionDef& md = w.mission.def();
        flagIcon(g, x, BY + 26);
        labelAt(std::string("LEVEL - ") + md.title, x + 22);
        float vw = valueAt(std::to_string(w.mission.index() + 1) + " / " + std::to_string(MissionManager::count()), x + 22, val);
        g.text(diffLabel(w.difficulty), x + 22 + vw + 10, VY + 7, 11, diffColor((int)w.difficulty), Align::Left, Font::Title);
    }

    
    const float R2 = BY + 62, RX = 1117.f;
    g.rect(12, R2, RX - 12, 1, lineCol);
    {
        const MissionDef& md = w.mission.def();
        float x = 24;
        x += g.text(md.title, x, R2 + 7, 12, C::Accent, Align::Left, Font::Title) + 10;
        g.text(w.mission.progressText(), x, R2 + 8, 12, val, Align::Left, Font::Plain);
        
        const int have[3] = {w.missionKills, w.pickupsCollected, w.barricadesPlaced}, need[3] = {10, 3, 2};
        const char* shortName[3] = {"Kills", "Pickups", "Barricades"};
        float bx = RX;
        for (int i = 2; i >= 0; --i) {
            bool done = (w.objectivesDone >> i) & 1u;
            std::string txt = std::string(shortName[i]) + " " + (done ? "+200" : std::to_string(std::min(have[i], need[i])) + "/" + std::to_string(need[i]));
            float tw = g.textWidth(txt, 11.5f, Font::Plain);
            bx -= tw;
            g.text(txt, bx, R2 + 8, 11.5f, done ? C::Good : lab, Align::Left, Font::Plain);
            bx -= 14;
            if (done) g.circle(bx + 5, R2 + 15, 4.5f, C::Good); else g.ring(bx + 5, R2 + 15, 4.f, 1.3f, C::Faint);
            bx -= 14;
        }
        g.text("BONUS", bx - 34, R2 + 8, 11, C::Accent, Align::Left, Font::Title);
    }
    {
        float y = R2 + 26;
        int alive = (int)w.zombies.size();
        std::string wave = "WAVE " + std::to_string(w.waveNumber) + "   " + std::to_string(alive) + (alive == 1 ? " zombie" : " zombies") +
                           (w.waves.empty() ? "   (final wave)" : "   next wave in " + std::to_string((int)std::ceil(std::max(0.f, w.waveTimer))) + " s");
        float x = 24;
        x += g.text(wave, x, y, 12, alive ? Col(255, 140, 120) : lab, Align::Left, Font::Plain) + 28;
        const MissionDef& md = w.mission.def();
        if (md.type == MissionType::SurviveThenReach) {          // Level 3: survival clock
            float left = w.mission.surviveLeft();
            std::string sv = "Survival Time: " + std::to_string((int)w.missionTime) + " sec" +
                             (left > 0.f ? "   (hold out " + std::to_string((int)std::ceil(left)) + " s)" : "   GO TO THE HELIPAD!");
            x += g.text(sv, x, y, 12, left > 0.f ? C::Accent : C::Good, Align::Left, Font::Title) + 28;
        }
        if (danger) {
            std::string d = std::string("DANGER: ") + kindName(w.threatKind) + " CLOSE";
            float dw = g.textWidth(d, 11.5f, Font::Title) + 18;
            g.roundRect(x, y - 2, dw, 20, 5, Col(200, 30, 30), Col(140, 10, 10));
            g.textMid(d, x + dw / 2, y + 8, 11.5f, C::White, Align::Center, Font::Title);
        }
        const PathStat& ps = w.useDijkstra ? w.dijStat : w.bfsStat;
        char buf[64];
        std::snprintf(buf, sizeof buf, "AI: %s  %.0f us   [B]", w.useDijkstra ? "DIJKSTRA" : "BFS", ps.avgUs);
        g.text(buf, RX, y, 12, C::Info, Align::Right, Font::Plain);
    }

    {
        const float MW = 141.f, MH = MW * WORLD_H / WORLD_W, MX = UI_W - MW - 9.f, MY = BY + 13.f;
        g.rect(MX - 9, BY + 8, 1, HUD_H - 16, lineCol);
        g.text("MINIMAP", MX + MW / 2, BY + 1, 10, lab, Align::Center, Font::Plain);
        world.drawMinimap(g, w, MX, MY, MW, MH);
    }
}

void drawCrosshair(Gfx& g, const Fx& fx) {
    g.ui();
    FPt m = g.mouseUi();
    float gap = 6.f + fx.recoil * 7.f, len = 7.f;
    const float dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
    for (int pass = 0; pass < 2; ++pass) {
        Col c = pass == 0 ? Col(0, 0, 0, 170) : Col(255, 255, 255, 235);
        float th = pass == 0 ? 3.6f : 1.8f;
        for (int i = 0; i < 4; ++i)
            g.line(m.x + dx[i] * gap, m.y + dy[i] * gap, m.x + dx[i] * (gap + len), m.y + dy[i] * (gap + len), th, c);
        g.circle(m.x, m.y, pass == 0 ? 2.4f : 1.3f, pass == 0 ? Col(0, 0, 0, 170) : C::Accent);
    }
}

void drawMenu(Gfx& g, const Art& a, bool canResume, bool hasSave, Difficulty diff, const std::string& musicLabel, bool effectsOn) {
    g.ui();
    const float t = (float)g.time;
    g.sprite(a.glow, UI_W / 2, 118, 900, 220, 0, Col(255, 120, 40, 40), SDL_BLENDMODE_ADD);
    g.textShadow("DYNAMIC NAVIGATION", UI_W / 2, 56, 58, C::White, Align::Center, Font::Title, 4.f);
    g.textShadow("& ENTITY MANAGEMENT SYSTEM", UI_W / 2, 136, 26, C::Accent, Align::Center, Font::Title, 3.f);
    g.text("Top-down zombie survival   \xC2\xB7   3-mission campaign   \xC2\xB7   C++17 + SDL2", UI_W / 2, 184, 15, C::Dim, Align::Center);

    const char* chips[] = {"Graph", "BFS", "Dijkstra", "Queue", "Min-Heap", "Linked List", "Stack", "BST"};
    float total = 0.f;
    for (const char* c : chips) total += g.textWidth(c, 12, Font::Title) + 22 + 8;
    float x = UI_W / 2 - (total - 8) / 2;
    for (const char* c : chips) {
        float cw = g.textWidth(c, 12, Font::Title) + 22;
        g.roundRect(x, 222, cw, 26, 13, Col(255, 255, 255, 18));
        g.roundOutline(x, 222, cw, 26, 13, Col(255, 176, 46, 70));
        g.textMid(c, x + cw / 2, 235, 12, Col(255, 214, 150), Align::Center, Font::Title);
        x += cw + 8;
    }

    g.button(menuButton(0), "START CAMPAIGN", true, 17);
    g.button(menuButton(1), "MULTIPLAYER  (2-PLAYER CO-OP)", true, 15);
    if (canResume) g.button(menuButton(2), "RESUME GAME", false, 16);
    else g.button(menuButton(2), hasSave ? "CONTINUE  (load save)" : "CONTINUE  (no save)", false, 15, hasSave);
    g.button(menuButton(3), std::string("DIFFICULTY:  ") + diffName(diff), false, 15);
    FRect d = menuButton(3);
    g.circle(d.x + d.w - 22, d.y + d.h / 2, 5, diffColor((int)diff));
    g.button(menuButton(4), "LEADERBOARD", false, 16);
    g.button(menuButton(5), "HOW TO PLAY", false, 16);
    g.button(menuButton(6), g.isFullscreen() ? "FULLSCREEN:  ON" : "FULLSCREEN:  OFF", false, 15);
    g.button(menuButton(7), musicLabel, false, 15);
    g.button(menuButton(8), effectsOn ? "SOUND EFFECTS:  ON" : "SOUND EFFECTS:  OFF", false, 15);
    g.button(menuButton(9), "EXIT", false, 16);

    float blink = 0.55f + 0.45f * std::sin(t * 3.f);
    g.text("ENTER  start      N  next song      X  sound effects      F11  fullscreen      F12  screenshot", UI_W / 2, 684, 12.5f, C::Faint.alpha(0.6f + 0.4f * blink), Align::Center);
}


void drawLeaderboardScreen(Gfx& g, const Leaderboard& board) {
    g.ui();
    g.textShadow("LEADERBOARD", UI_W / 2, 36, 42, C::Accent, Align::Center, Font::Title, 3.f);
    g.text("Binary search tree, reverse in-order traversal = best first", UI_W / 2, 92, 13, C::Dim, Align::Center);
    const float X = 290, W = 700;
    g.panel(X, 124, W, 494, 12);
    const float cR = X + 48, cN = X + 110, cD = X + 380, cS = X + 560, cT = X + 670;
    g.text("RANK", cR, 140, 11, C::Faint, Align::Center, Font::Title);
    g.text("PLAYER", cN, 140, 11, C::Faint, Align::Left, Font::Title);
    g.text("DIFFICULTY", cD, 140, 11, C::Faint, Align::Left, Font::Title);
    g.text("SCORE", cS, 140, 11, C::Faint, Align::Right, Font::Title);
    g.text("TIME", cT, 140, 11, C::Faint, Align::Right, Font::Title);
    g.rect(X + 20, 162, W - 40, 1, Col(255, 255, 255, 24));
    int rank = 1;
    const Col medal[3] = {Col(255, 200, 60), Col(200, 206, 216), Col(214, 140, 80)};
    auto rows = board.top(10);
    for (const ScoreRow& r : rows) {
        float y = 168 + (rank - 1) * 44.f, cy = y + 21;
        bool me = r.name == "Player" || r.name == "Co-op Team";
        if (me) g.roundRect(X + 14, y + 3, W - 28, 38, 8, Col(90, 225, 130, 22));
        else if (rank % 2 == 0) g.roundRect(X + 14, y + 3, W - 28, 38, 8, Col(255, 255, 255, 8));
        if (rank <= 3) { g.circle(cR, cy, 14, medal[rank - 1]); g.textMid(std::to_string(rank), cR, cy, 14, Col(30, 24, 10), Align::Center, Font::Title); }
        else g.textMid(std::to_string(rank), cR, cy, 15, C::Dim, Align::Center, Font::Title);
        g.textMid(r.name, cN, cy, 17, me ? C::Good : C::White, Align::Left, Font::Title);
        int d = std::clamp(r.difficulty, 0, 2);
        float pw = g.textWidth(diffName((Difficulty)d), 11, Font::Title) + 20;
        g.roundRect(cD, cy - 11, pw, 22, 11, diffColor(d).alpha(0.18f));
        g.textMid(diffName((Difficulty)d), cD + pw / 2, cy, 11, diffColor(d), Align::Center, Font::Title);
        g.textMid(std::to_string(r.score), cS, cy, 18, rank == 1 ? medal[0] : C::White, Align::Right, Font::Title);
        g.textMid(mmss(r.secs), cT, cy, 15, C::Dim, Align::Right);
        ++rank;
    }
    if (rows.empty()) g.text("No runs yet - finish a campaign to get on the board!", UI_W / 2, 360, 16, C::Dim, Align::Center);
    g.text("Sample_* rows are placeholder data. Finished runs are saved to leaderboard.txt", UI_W / 2, 624, 12, C::Faint, Align::Center);
    g.button(backButton(), "BACK  (Esc)", false, 16);
}

void drawHelp(Gfx& g, const Art& a) {
    g.ui();
    g.textShadow("HOW TO PLAY", UI_W / 2, 24, 40, C::Accent, Align::Center, Font::Title, 3.f);
    // ---- controls
    g.panel(56, 90, 560, 540, 12);
    g.text("CONTROLS", 80, 104, 13, C::Accent, Align::Left, Font::Title);
    const char* keys[][2] = {
        {"W A S D", "Move  (arrow keys work too)"}, {"SHIFT", "Sprint - uses stamina"}, {"LMB", "Shoot"},
        {"1 / 2", "Rifle / Pistol"}, {"RMB", "Place / remove a barricade on a road"}, {"E", "Throw a grenade (it hurts you too!)"},
        {"U", "Undo last barricade action  (stack)"}, {"3", "Use a medkit"}, {"F", "Flashlight on / off"},
        {"B", "Zombie AI: BFS <-> Dijkstra"}, {"TAB", "Show every zombie's path"}, {"F5 / F9", "Save / Load"},
        {"ESC / P", "Pause"}, {"N", "Next song  /  music off"}, {"X", "Sound effects on / off"}, {"F11 / F12", "Fullscreen / Screenshot"}};
    float y = 146;
    for (auto& k : keys) {
        keycap(g, k[0], 80, y);
        g.textMid(k[1], 220, y, 14, C::Text);
        y += 30.f;
    }
    
    g.panel(664, 90, 560, 540, 12);
    g.text("CAMPAIGN", 688, 104, 13, C::Accent, Align::Left, Font::Title);
    g.text("1  Safe Zone      2  Fuel Run      3  Last Stand", 688, 126, 14, C::White, Align::Left, Font::Title);
    g.text("Reach the zone  >  collect 3 fuel cans  >  survive 75 s, then the helipad", 688, 148, 12, C::Dim);
    g.text("ZOMBIES", 688, 182, 13, C::Accent, Align::Left, Font::Title);
    const char* zd[5][2] = {{"Walker", "Slow and steady"}, {"Runner", "Fast but fragile"}, {"Tank", "Tough, hits hard, chews barricades"},
                            {"Spitter", "Keeps its distance and spits acid"}, {"BOSS", "30 HP - charges every 6 seconds"}};
    for (int i = 0; i < 5; ++i) {
        float cy = 224 + i * 44.f;
        float s = i == 4 ? 46.f : 40.f;
        g.circle(712, cy, 19, Col(255, 255, 255, 10));
        g.sprite(a.zombie[i], 712, cy, s * 1.3f, s * 1.3f, -90.f);
        g.text(zd[i][0], 744, cy - 18, 15, i == 4 ? C::Danger : C::White, Align::Left, Font::Title);
        g.text(zd[i][1], 744, cy + 2, 12.5f, C::Dim);
    }
    g.text("RULES OF THE STREET", 688, 446, 13, C::Accent, Align::Left, Font::Title);
    const char* tips[] = {"Gunfire and explosions are NOISE - far zombies come hunting.",
                          "Sealed in by barricades? Zombies chew right through them.",
                          "Rubble slows everyone. Dijkstra avoids it, BFS does not.",
                          "Quick kills build a COMBO multiplier, up to x5.",
                          "Bonus objectives +200 each. Air drops every 45 s."};
    for (int i = 0; i < 5; ++i) {
        g.circle(694, 476 + i * 28.f, 2.5f, C::Accent);
        g.textMid(tips[i], 706, 476 + i * 28.f, 13, C::Text);
    }
    g.button(backButton(), "BACK  (Esc)", false, 16);
}
void drawOverlay(Gfx& g, Screen s, const Game& w) {
    if (s == Screen::Playing) return;
    g.ui();
    const float t = (float)g.time;
    g.rectGrad(0, 0, UI_W, UI_H, Col(4, 6, 10, 150), Col(4, 6, 10, 215));
    if (s == Screen::Lost) g.rect(0, 0, UI_W, UI_H, Col(90, 0, 0, alpha8(70 + 20 * std::sin(t * 2.f))));

    const bool paused = s == Screen::Paused;
    const float W = 600, H = paused ? 250 : 340, X = UI_W / 2 - W / 2, Y = paused ? 220 : 170;
    std::string title = paused ? "PAUSED" : s == Screen::MissionClear ? "MISSION COMPLETE" : s == Screen::Won ? "CAMPAIGN COMPLETE" : "GAME OVER";
    Col tc = s == Screen::Lost ? Col(255, 80, 70) : s == Screen::Won ? Col(255, 210, 90) : s == Screen::MissionClear ? C::Good : C::White;
    g.panel(X, Y, W, H, 14);
    g.rectHGrad(X + 40, Y + 1, (W - 80) / 2, 2, tc.alpha(0.f), tc);
    g.rectHGrad(X + W / 2, Y + 1, (W - 80) / 2, 2, tc, tc.alpha(0.f));
    g.textShadow(title, UI_W / 2, Y + 22, 40, tc, Align::Center, Font::Title, 3.f);

    auto hint = [&](float cy, std::initializer_list<std::pair<const char*, const char*>> items) {
        float total = 0.f;
        for (auto& it : items) total += g.textWidth(it.first, 12, Font::Title) + 14 + 8 + g.textWidth(it.second, 13) + 28;
        float x = UI_W / 2 - (total - 28) / 2;
        for (auto& it : items) {
            x += keycap(g, it.first, x, cy) + 8;
            x += g.textMid(it.second, x, cy, 13, C::Dim) + 28;
        }
    };

    if (paused) {
        g.text("LEVEL " + std::to_string(w.mission.index() + 1) + "  -  " + w.mission.def().title, UI_W / 2, Y + 86, 17, C::Accent, Align::Center, Font::Title);
        g.text(w.mission.def().brief, UI_W / 2, Y + 112, 13.5f, C::Text, Align::Center);
        g.text(w.mission.progressText(), UI_W / 2, Y + 134, 13.5f, C::Dim, Align::Center);
        hint(Y + 190, {{"ESC", "resume"}, {"F5", "save"}, {"F9", "load"}, {"M", "main menu"}});
        return;
    }
    // stats grid
    int done = 0;
    for (int i = 0; i < 3; ++i) if (w.objectivesDone & (1u << i)) ++done;
    const std::string vals[4] = {std::to_string(w.score), std::to_string(w.kills), mmss((int)w.gameTime), std::to_string(done) + "/3"};
    const char* labels[4] = {"SCORE", "KILLS", "TIME", "BONUS"};
    for (int i = 0; i < 4; ++i) {
        float cx = X + 75 + i * 150.f;
        g.roundRect(cx - 64, Y + 84, 128, 66, 10, Col(255, 255, 255, 12));
        g.text(labels[i], cx, Y + 92, 11, C::Faint, Align::Center, Font::Title);
        g.text(vals[i], cx, Y + 108, 24, i == 0 ? C::Info : C::White, Align::Center, Font::Title);
    }
    g.text(std::string("Difficulty: ") + diffName(w.difficulty), UI_W / 2, Y + 162, 13, diffColor((int)w.difficulty), Align::Center, Font::Title);

    if (s == Screen::MissionClear) {
        const MissionDef& next = MissionManager::defOf(w.mission.index() + 1);
        g.text("NEXT:  MISSION " + std::to_string(w.mission.index() + 2) + "  -  " + next.title, UI_W / 2, Y + 196, 18, C::Accent, Align::Center, Font::Title);
        g.text(next.brief, UI_W / 2, Y + 224, 13.5f, C::Text, Align::Center);
        hint(Y + 290, {{"ENTER", "next mission"}, {"ESC", "main menu"}});
    } else {
        g.text(s == Screen::Won ? "The helicopter lifts off. You made it out of the city." : "The city claims another survivor.",
               UI_W / 2, Y + 204, 15, C::Text, Align::Center);
        g.text("Your run was added to the leaderboard.", UI_W / 2, Y + 230, 12.5f, C::Dim, Align::Center);
        hint(Y + 290, {{"ENTER", "play again"}, {"ESC", "main menu"}});
    }
}

void drawMultiplayer(Gfx& g, const Art& a, bool gamepadConnected, Difficulty diff) {
    g.ui();
    g.textShadow("MULTIPLAYER", UI_W / 2, 22, 42, C::Accent, Align::Center, Font::Title, 3.f);
    g.text("2-player co-op on one computer  \xC2\xB7  share one keyboard, or plug in a gamepad for player 2", UI_W / 2, 78, 14, C::Dim, Align::Center);

    struct Row { const char* key; const char* what; };
    auto card = [&](float x, int team, const char* title, const Row* rows, int n, const char* note) {
        const Col tc = team == 0 ? Col(90, 205, 255) : Col(255, 110, 200);
        g.panel(x, 112, 480, 400, 14);
        g.rectHGrad(x + 30, 113, 210, 2, tc.alpha(0.f), tc);
        g.rectHGrad(x + 240, 113, 210, 2, tc, tc.alpha(0.f));
        g.sprite(a.glow, x + 70, 172, 110, 110, 0, tc.alpha(0.25f), SDL_BLENDMODE_ADD);
        g.ring(x + 70, 172, 34, 3, tc);
        g.sprite(a.player[team][0], x + 70, 172, 92, 92, -90.f);
        g.text(title, x + 124, 138, 26, tc, Align::Left, Font::Title);
        g.text(note, x + 124, 174, 12.5f, C::Dim);
        float y = 236;
        for (int i = 0; i < n; ++i, y += 31.f) {
            float kw = g.textWidth(rows[i].key, 11.5f, Font::Title) + 14, h = 23;
            g.roundRect(x + 28, y - h / 2 + 2, kw, h, 5, Col(0, 0, 0, 120));
            g.roundRect(x + 28, y - h / 2, kw, h, 5, Col(70, 78, 92), Col(46, 52, 64));
            g.textMid(rows[i].key, x + 28 + kw / 2, y, 11.5f, C::White, Align::Center, Font::Title);
            g.textMid(rows[i].what, x + 196, y, 13.5f, C::Text);
        }
    };
    const Row p1[] = {{"W A S D", "Move"}, {"MOUSE", "Aim"}, {"LEFT CLICK", "Shoot"}, {"SHIFT", "Sprint"},
                      {"1 / 2", "Rifle / Pistol"}, {"E", "Grenade (at the mouse)"}, {"3", "Medkit"}, {"RIGHT CLICK", "Barricade"}, {"F", "Flashlight"}};
    const Row p2[] = {{"ARROW KEYS", "Move"}, {"AUTO", "Aim locks on to zombies in front"}, {"RIGHT CTRL", "Shoot   (or NUM 0)"},
                      {"RIGHT SHIFT", "Sprint"}, {",", "Switch weapon   (or NUM 3)"}, {"/", "Grenade   (or NUM 1)"},
                      {".", "Medkit   (or NUM 2)"}, {"ENTER", "Barricade in front"}, {"GAMEPAD", gamepadConnected ? "Connected - ready!" : "Optional: plug one in"}};
    card(140, 0, "PLAYER 1", p1, 9, "Keyboard + mouse (left side)");
    card(660, 1, "PLAYER 2", p2, 9, gamepadConnected ? "Gamepad  (or the right side of the keyboard)" : "Right side of the keyboard");

    g.panel(140, 524, 1000, 98, 12);
    g.text("TEAM RULES", 164, 534, 12, C::Accent, Align::Left, Font::Title);
    const char* rules[] = {"Ammo, medkits, grenades and barriers are shared by the team.",
                           "Zombies hunt whoever is closer.  Stay together: the camera keeps you both on screen.",
                           "At 0 health a player is DOWN - stand next to them for 2.5 s to revive.  The run ends when both are down."};
    for (int i = 0; i < 3; ++i) {
        g.circle(170, 562 + i * 20.f, 2.5f, C::Accent);
        g.textMid(rules[i], 182, 562 + i * 20.f, 13, C::Text);
    }
    g.text(std::string("Difficulty: ") + diffName(diff) + "  (change it in the main menu)", 1116, 534, 11.5f, diffColor((int)diff), Align::Right, Font::Title);
    g.button(coopStartButton(), "START CO-OP", true, 17);
    g.button(coopBackButton(), "BACK  (Esc)", false, 16);
    g.text("ENTER  start co-op", UI_W / 2, 694, 12, C::Faint, Align::Center);
}

void drawToast(Gfx& g, const std::string& message, float timeLeft) {
    g.ui();
    float k = clamp01(timeLeft / 0.3f) * clamp01((2.f - timeLeft) / 0.15f + 0.2f);
    float tw = g.textWidth(message, 14, Font::Title) + 40, cx = VIEW_W / 2;   // centred in the game view
    float y = 24 - (1.f - k) * 10.f;
    g.roundRect(cx - tw / 2, y + 3, tw, 32, 16, Col(0, 0, 0, alpha8(100 * k)));
    g.roundRect(cx - tw / 2, y, tw, 32, 16, Col(34, 40, 52, alpha8(240 * k)), Col(20, 24, 32, alpha8(240 * k)));
    g.roundOutline(cx - tw / 2, y, tw, 32, 16, C::Accent.alpha(k));
    g.textMid(message, cx, y + 16, 14, C::White.alpha(k), Align::Center, Font::Title);
}
