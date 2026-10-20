#include <SFML/Graphics.hpp>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using Col = sf::Color;
using Vec = sf::Vector2f;

constexpr float WIN_W = 1280.f, WIN_H = 720.f, VIEW_W = 960.f, T = 32.f;
constexpr float OX = 48.f, OY = -8.f;  // fixed "camera": city tile (13,11) sits at the screen centre

sf::RenderWindow* gWin = nullptr;
sf::Font gFont;
bool gFontOk = false;


void text(const std::string& s, float x, float y, unsigned size, Col c = Col::White, int align = 0) {
    if (!gFontOk) return;
    sf::Text t(gFont, s, size);
    t.setFillColor(c);
    float w = t.getLocalBounds().size.x;
    t.setOrigin({align == 1 ? w / 2.f : (align == 2 ? w : 0.f), 0.f});
    t.setPosition({x, y});
    gWin->draw(t);
}
void rect(float x, float y, float w, float h, Col fill, Col line = Col::Transparent, float th = 0.f) {
    sf::RectangleShape r({w, h});
    r.setPosition({x, y});
    r.setFillColor(fill);
    r.setOutlineColor(line);
    r.setOutlineThickness(th);
    gWin->draw(r);
}
void circle(Vec p, float rad, Col fill, Col line = Col::Transparent, float th = 0.f) {
    sf::CircleShape c(rad);
    c.setOrigin({rad, rad});
    c.setPosition(p);
    c.setFillColor(fill);
    c.setOutlineColor(line);
    c.setOutlineThickness(th);
    gWin->draw(c);
}
void panel(float x, float y, float w, float h, const std::string& title = "") {
    rect(x, y, w, h, Col(14, 18, 20, 230), Col(60, 70, 76), 1.f);
    if (!title.empty()) text(title, x + w / 2.f, y + 4.f, 14, Col(230, 170, 40), 1);
}
void button(const std::string& label, float x, float y, float w, float h, bool primary = false, unsigned fs = 18) {
    rect(x, y, w, h, primary ? Col(30, 110, 60) : Col(28, 32, 38), Col(110, 120, 130), 1.f);
    text(label, x + w / 2.f, y + (h - fs) / 2.f - 2.f, fs, Col::White, 1);
}


bool isRoad(int x, int y) { return x >= 0 && y >= 0 && (y % 10 < 2 || x % 12 < 2); }
bool isBarricade(int x, int y) { return (x == 12 && y == 14) || (x == 16 && y == 11); }
Vec tp(float c, float r) { return {c * T + T / 2.f + OX, r * T + T / 2.f + OY}; }


const std::vector<Vec> ZOMBIES = {{13, 6}, {8, 10}, {19, 11}, {13, 16}, {24, 10}, {12, 20}};
const std::vector<std::vector<Vec>> ZOMBIE_PATHS = {
    {{13, 6}, {13, 7}, {13, 8}, {13, 9}, {13, 10}, {13, 11}},
    {{8, 10}, {9, 10}, {10, 10}, {11, 10}, {12, 10}, {13, 10}, {13, 11}},
    {{19, 11}, {19, 10}, {18, 10}, {17, 10}, {16, 10}, {15, 10}, {14, 10}, {13, 10}, {13, 11}},
    {{13, 16}, {13, 15}, {13, 14}, {13, 13}, {13, 12}, {13, 11}}};
const std::vector<Vec> ROUTE = {{14, 11}, {15, 11}, {15, 10}, {16, 10}, {17, 10}, {18, 10}, {19, 10}, {20, 10},
                                {21, 10}, {22, 10}, {23, 10}, {24, 10}, {25, 10}, {26, 10}, {27, 10}, {28, 10}};

void drawCity(bool showPaths) {
    rect(0, 0, VIEW_W, WIN_H, Col(10, 10, 12));
    for (int y = 0; y < 23; ++y)
        for (int x = 0; x < 30; ++x) {
            Col c;
            if (isBarricade(x, y)) c = Col(150, 95, 40);
            else if (isRoad(x, y)) { int v = 56 + ((x + y) & 1) * 4; c = Col(v, v + 2, v + 8); }
            else {
                bool interior = !isRoad(x + 1, y) && !isRoad(x - 1, y) && !isRoad(x, y + 1) && !isRoad(x, y - 1);
                c = interior ? Col(74, 62, 52) : Col(104, 104, 112);
            }
            rect(x * T + OX, y * T + OY, T, T, c);
        }

    
    for (auto& r : ROUTE) circle(tp(r.x, r.y), 3.f, Col(240, 210, 60, 170));

    
    if (showPaths)
        for (auto& path : ZOMBIE_PATHS) {
            sf::VertexArray va(sf::PrimitiveType::LineStrip);
            for (auto& p : path) va.append(sf::Vertex{tp(p.x, p.y), Col(255, 80, 80)});
            gWin->draw(va);
            for (auto& p : path) circle(tp(p.x, p.y), 3.f, Col(255, 120, 120));
        }

    Vec player = tp(13, 11);

    
    sf::ConvexShape cone(3);
    cone.setPoint(0, player);
    cone.setPoint(1, player + Vec{std::cos(-0.4f), std::sin(-0.4f)} * 260.f);
    cone.setPoint(2, player + Vec{std::cos(0.4f), std::sin(0.4f)} * 260.f);
    cone.setFillColor(Col(255, 240, 150, 45));
    gWin->draw(cone);

  
    Vec hp = tp(15, 10);
    rect(hp.x - 8, hp.y - 8, 16, 16, Col(240, 240, 240));
    rect(hp.x - 6, hp.y - 2, 12, 4, Col(210, 40, 40));
    rect(hp.x - 2, hp.y - 6, 4, 12, Col(210, 40, 40));
    Vec am = tp(13, 8);
    rect(am.x - 8, am.y - 8, 16, 16, Col(90, 110, 60), Col(230, 200, 60), 2.f);

   
    for (auto& z : ZOMBIES) {
        Vec p = tp(z.x, z.y);
        circle(p, 16.f, Col::Transparent, Col(200, 40, 40, 200), 2.f);
        circle(p, 12.f, Col(110, 130, 80));
    }

 
    circle(player + Vec{26.f, 0.f}, 2.5f, Col(255, 230, 120));
    circle(player + Vec{58.f, 0.f}, 2.5f, Col(255, 230, 120));

   
    circle(player, 11.f, Col(70, 190, 110), Col(20, 60, 30), 2.f);
    rect(player.x, player.y - 2.f, 20.f, 4.f, Col(30, 30, 30));
}


sf::VertexArray gMini(sf::PrimitiveType::Triangles);

void buildMinimap() {
    const float mx = VIEW_W - 190.f, my = 26.f, s = 3.f;
    for (int y = 0; y < 40; ++y)
        for (int x = 0; x < 60; ++x) {
            if (!isRoad(x, y)) continue;
            Vec a{mx + x * s, my + y * s}, b{a.x + s, a.y}, c{a.x + s, a.y + s}, d{a.x, a.y + s};
            for (Vec v : {a, b, c, a, c, d}) gMini.append(sf::Vertex{v, Col(210, 210, 215)});
        }
}

void drawHud() {
    // player panel
    panel(10, 10, 230, 100);
    circle({46.f, 52.f}, 26.f, Col(224, 190, 150));
    rect(26, 24, 40, 12, Col(40, 30, 25));
    rect(35, 50, 5, 5, Col(20, 20, 20));
    rect(52, 50, 5, 5, Col(20, 20, 20));
    text("HEALTH", 84, 14, 12);
    text("80/100", 232, 14, 12, Col::White, 2);
    rect(84, 32, 148, 12, Col(60, 20, 20));
    rect(84, 32, 148 * 0.80f, 12, Col(190, 40, 40));
    text("AMMO", 84, 50, 12);
    text("42/60", 232, 50, 12, Col::White, 2);
    rect(84, 68, 148, 12, Col(60, 50, 15));
    rect(84, 68, 148 * 0.70f, 12, Col(220, 170, 30));
    text("SCORE : 1250", 20, 86, 16, Col(120, 230, 255));

    
    panel(10, 120, 230, 96);
    text("CURRENT MISSION", 20, 126, 14, Col(230, 170, 40));
    text("Reach the Safe Zone", 20, 148, 15);
    text("Route: 52 tiles", 20, 172, 13, Col(210, 210, 210));
    text("Wave timer: 11s | 01:15", 20, 192, 12, Col(150, 150, 150));

   
    const float mx = VIEW_W - 190.f, my = 26.f;
    text("MINIMAP", mx + 90.f, 6.f, 13, Col::White, 1);
    rect(mx, my, 180, 120, Col(0, 0, 0, 220), Col::White, 1.f);
    gWin->draw(gMini);
    rect(mx + 12 * 3.f, my + 14 * 3.f, 3, 3, Col(230, 70, 50));  // barricades
    rect(mx + 16 * 3.f, my + 11 * 3.f, 3, 3, Col(230, 70, 50));
    for (auto& z : ZOMBIES) rect(mx + z.x * 3.f - 1, my + z.y * 3.f - 1, 3, 3, Col(255, 60, 60));
    rect(mx + 49 * 3.f - 3, my + 31 * 3.f - 3, 6, 6, Col(240, 210, 60));
    rect(mx + 13 * 3.f - 2, my + 11 * 3.f - 2, 5, 5, Col(80, 255, 120));

    
    const char* names[5] = {"Rifle", "Pistol", "Medkit", "Barrier", "Torch"};
    const char* counts[5] = {"42", "30", "2", "3", "ON"};
    float x0 = (VIEW_W - 314.f) / 2.f, y0 = 720.f - 76.f;
    for (int i = 0; i < 5; ++i) {
        bool sel = (i == 0 || i == 4);
        rect(x0 + i * 64.f, y0, 58, 62, Col(14, 18, 20, 230), sel ? Col(230, 200, 60) : Col(80, 90, 96), sel ? 2.f : 1.f);
        text(std::to_string(i + 1), x0 + i * 64.f + 4.f, y0 + 2.f, 12, Col(180, 180, 180));
        text(names[i], x0 + i * 64.f + 29.f, y0 + 20.f, 12, Col::White, 1);
        text(counts[i], x0 + i * 64.f + 29.f, y0 + 38.f, 16, Col(230, 200, 60), 1);
    }
}

void drawSidebar() {
    const float X = VIEW_W;
    rect(X, 0, 320, WIN_H, Col(8, 10, 12));

    panel(X + 8, 8, 304, 70, "MAIN MENU");
    button("PAUSE", X + 16, 40, 90, 30, false, 13);
    button("RESTART", X + 114, 40, 90, 30, false, 13);
    button("MENU", X + 212, 40, 90, 30, false, 13);

    panel(X + 8, 86, 304, 100, "INVENTORY (linked list)");
    const char* inv[5] = {"Rifle", "Pistol", "Medkit", "Barrier", "Torch"};
    const char* cnt[5] = {"42", "30", "2", "3", "ON"};
    for (int i = 0; i < 5; ++i) {
        float cx = X + 16.f + i * 58.f;
        rect(cx, 112, 54, 66, Col(24, 28, 32), Col(80, 90, 96), 1.f);
        text(inv[i], cx + 27.f, 116.f, 11, Col(190, 190, 190), 1);
        text(cnt[i], cx + 27.f, 142.f, 16, Col::White, 1);
    }

    panel(X + 8, 194, 304, 186, "LEADERBOARD (BST)");
    text("RANK  PLAYER", X + 16, 218, 11, Col(160, 160, 160));
    text("SCORE", X + 236, 218, 11, Col(160, 160, 160), 2);
    text("TIME", X + 300, 218, 11, Col(160, 160, 160), 2);
    const char* names[5] = {"Sample_A", "Sample_B", "Sample_C", "Sample_D", "Sample_E"};
    const char* score[5] = {"18550", "14890", "9840", "7620", "4200"};
    const char* tm[5] = {"85:30", "65:12", "43:21", "33:10", "25:00"};
    for (int i = 0; i < 5; ++i) {
        float y = 238.f + i * 22.f;
        text(std::to_string(i + 1), X + 24, y, 13);
        text(names[i], X + 64, y, 13);
        text(score[i], X + 236, y, 13, Col::White, 2);
        text(tm[i], X + 300, y, 13, Col::White, 2);
    }
    rect(X + 12, 354, 296, 22, Col(30, 70, 40));
    text("Player (live)", X + 24, 357, 13, Col(160, 255, 170));
    text("1250", X + 236, 357, 13, Col(160, 255, 170), 2);
    text("01:15", X + 300, 357, 13, Col(160, 255, 170), 2);

    panel(X + 8, 388, 304, 324, "DATA STRUCTURES (example values)");
    const char* lines[] = {
        "Graph: 2400 nodes (grid, 4-neighbour)", "BFS: avg 80 us/call, 120 calls", "Route to safe zone: 52 tiles",
        "Queue (waves): 4 left, next = 10",      "Priority Queue: nearest threat 160 px",
        "Linked List: 5 inventory items",        "Stack top: Barricade placed",
        "BST (multiset): 4 scores",              "Zombies: 6 | bullets: 2"};
    float y = 414.f;
    for (auto l : lines) { text(l, X + 18, y, 13, Col(210, 210, 215)); y += 22.f; }
    text("WASD move | LMB shoot | RMB barricade", X + 18, y + 6.f, 12, Col(140, 140, 140));
    text("1/2 weapon | 3 medkit | 5 torch | G paths", X + 18, y + 24.f, 12, Col(140, 140, 140));
}


void screenGame(bool paths, bool paused) {
    drawCity(paths);
    drawHud();
    if (paused) {
        rect(0, 0, VIEW_W, WIN_H, Col(0, 0, 0, 170));
        text("PAUSED", VIEW_W / 2.f, 250.f, 52, Col::White, 1);
        text("Press ESC or P to resume", VIEW_W / 2.f, 330.f, 22, Col(210, 210, 210), 1);
    }
    drawSidebar();
}

void screenMenu() {
    text("DYNAMIC NAVIGATION", WIN_W / 2.f, 90.f, 56, Col::White, 1);
    text("& ENTITY MANAGEMENT SYSTEM", WIN_W / 2.f, 158.f, 40, Col(230, 70, 60), 1);
    text("2D Top-Down Survival Prototype  |  C++ + SFML  |  BFS, Queue, Priority Queue, Linked List, Stack, BST",
         WIN_W / 2.f, 236.f, 15, Col(60, 220, 120), 1);
    const char* b[5] = {"START GAME", "GUI PREVIEW", "LEADERBOARD", "HOW TO PLAY", "EXIT"};
    for (int i = 0; i < 5; ++i) button(b[i], 540, 300.f + i * 54.f, 200, 42, i == 0);
}

void screenLeaderboard() {
    text("LEADERBOARD", WIN_W / 2.f, 50.f, 40, Col(230, 170, 40), 1);
    text("RANK", 380, 130, 16, Col(160, 160, 160));
    text("PLAYER", 460, 130, 16, Col(160, 160, 160));
    text("SCORE", 780, 130, 16, Col(160, 160, 160), 2);
    text("TIME", 900, 130, 16, Col(160, 160, 160), 2);
    const char* score[8] = {"18550", "14890", "12640", "9840", "7620", "4200", "2450", "1100"};
    const char* tm[8] = {"85:30", "65:12", "58:45", "43:21", "33:10", "25:00", "12:40", "06:05"};
    for (int i = 0; i < 8; ++i) {
        float y = 165.f + i * 40.f;
        text(std::to_string(i + 1), 380, y, 20);
        text(std::string("Sample_") + char('A' + i), 460, y, 20);
        text(score[i], 780, y, 20, Col::White, 2);
        text(tm[i], 900, y, 20, Col::White, 2);
    }
    text("Placeholder data for the interface design.", WIN_W / 2.f, 590.f, 14, Col(140, 140, 140), 1);
    button("BACK (Esc)", 540, 640, 200, 40);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "Dynamic Navigation and Entity Management System - GUI mockup",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(30);
    gWin = &window;

    const char* fonts[] = {"assets/font.ttf", "C:/Windows/Fonts/arial.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/System/Library/Fonts/Supplemental/Arial.ttf"};
    for (auto f : fonts)
        if (gFont.openFromFile(f)) { gFontOk = true; break; }
    buildMinimap();

    int screen = 2;  // 1 menu, 2 gameplay, 3 pathfinding, 4 paused, 5 leaderboard
    while (window.isOpen()) {
        while (const std::optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                using K = sf::Keyboard::Key;
                if (k->code == K::Escape) window.close();
                if (k->code == K::Num1) screen = 1;
                if (k->code == K::Num2) screen = 2;
                if (k->code == K::Num3) screen = 3;
                if (k->code == K::Num4) screen = 4;
                if (k->code == K::Num5) screen = 5;
                if (k->code == K::Right) screen = screen % 5 + 1;
                if (k->code == K::Left) screen = (screen + 3) % 5 + 1;
            }
        }

        window.clear(Col(12, 14, 16));
        switch (screen) {
        case 1: screenMenu(); break;
        case 2: screenGame(false, false); break;
        case 3: screenGame(true, false); break;
        case 4: screenGame(false, true); break;
        case 5: screenLeaderboard(); break;
        }
        text("GUI MOCKUP - static, no game logic   [1-5 / arrows: switch screen]", 8, WIN_H - 14.f, 11, Col(240, 210, 60));
        window.display();
    }
    return 0;
}
