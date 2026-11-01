#include "Screens.h"

void drawMenu(Gfx& g, bool hasSave, Difficulty diff) {
    g.beginUi();
    g.text("DYNAMIC NAVIGATION", UI_W / 2.f, 50.f, 56, Col::White, 1);
    g.text("& ENTITY MANAGEMENT SYSTEM", UI_W / 2.f, 118.f, 40, Col(230, 70, 60), 1);
    g.text("2D Top-Down Survival  |  3-Mission Campaign  |  C++ + SFML", UI_W / 2.f, 184.f, 16, Col(60, 220, 120), 1);
    g.text("Graph, BFS, Dijkstra, Queue, Priority Queue, Linked List, Stack, BST", UI_W / 2.f, 208.f, 14, Col(150, 150, 150), 1);

    g.button(menuButton(0), "START CAMPAIGN", true);
    g.button(menuButton(1), hasSave ? "CONTINUE" : "CONTINUE (no save)", false, hasSave ? 18 : 14);
    g.button(menuButton(2), std::string("DIFFICULTY: ") + diffName(diff), false, 16);
    g.button(menuButton(3), "LEADERBOARD");
    g.button(menuButton(4), "HOW TO PLAY");
    g.button(menuButton(5), g.isFullscreen() ? "FULLSCREEN: ON" : "FULLSCREEN: OFF", false, 17);
    g.button(menuButton(6), "EXIT");
    g.text("F11 toggles fullscreen at any time", UI_W / 2.f, 610.f, 13, Col(140, 140, 140), 1);
}

void drawLeaderboardScreen(Gfx& g, const Leaderboard& board) {
    g.beginUi();
    g.text("LEADERBOARD", UI_W / 2.f, 50.f, 40, Col(230, 170, 40), 1);
    g.text("RANK", 340, 130, 16, Col(160, 160, 160));
    g.text("PLAYER", 420, 130, 16, Col(160, 160, 160));
    g.text("LEVEL", 700, 130, 16, Col(160, 160, 160));
    g.text("SCORE", 840, 130, 16, Col(160, 160, 160), 2);
    g.text("TIME", 940, 130, 16, Col(160, 160, 160), 2);
    int rank = 1;
    for (const ScoreRow& r : board.top(10)) {
        float y = 165.f + (rank - 1) * 40.f;
        Col c = r.name == "Player" ? Col(120, 230, 140) : Col::White;
        g.text(std::to_string(rank), 340, y, 20, c);
        g.text(r.name, 420, y, 20, c);
        g.text(diffName((Difficulty)(r.difficulty < 0 ? 0 : r.difficulty > 2 ? 2 : r.difficulty)), 700, y, 18, c);
        g.text(std::to_string(r.score), 840, y, 20, c, 2);
        g.text(mmss(r.secs), 940, y, 20, c, 2);
        ++rank;
    }
    g.text("Sample_* rows are placeholder data. Finished runs are saved to leaderboard.txt.", UI_W / 2.f, 590.f, 14, Col(140, 140, 140), 1);
    g.button(backButton(), "BACK (Esc)");
}

void drawHelp(Gfx& g) {
    g.beginUi();
    g.text("HOW TO PLAY", UI_W / 2.f, 30.f, 40, Col(230, 170, 40), 1);
    const char* lines[] = {
        "CAMPAIGN: 1 Safe Zone  ->  2 Fuel Run (collect 3 fuel cans)  ->  3 Last Stand (survive, then reach the helipad)",
        "WASD / arrows ....... move          SHIFT ....... sprint (uses stamina)",
        "Left mouse .......... shoot         1 / 2 ....... Rifle / Pistol",
        "Right mouse ......... place / remove a barricade on a road tile",
        "E ................... throw a grenade at the mouse (careful: it hurts you too)",
        "U ................... undo the last barricade action   (stack)",
        "3 / 5 ............... medkit / flashlight",
        "B ................... switch zombie AI between BFS and Dijkstra",
        "TAB ................. show every zombie's path        F5 / F9 .... save / load",
        "F11 ................. fullscreen on/off               ESC / P .... pause",
        "",
        "Zombies: Walker | Runner (fast) | Tank (tough) | Spitter (spits acid from range) | BOSS (charges)",
        "NOISE: shots and explosions are heard - zombies far away start hunting you (rifle is louder than pistol).",
        "Zombies with no route to you CHEW THROUGH your barricades. Rubble slows everyone; Dijkstra avoids it, BFS does not.",
        "Kill streaks build a COMBO multiplier (up to x5). Bonus objectives give +200 each. Air drops arrive every 45 s."};
    float y = 95.f;
    for (const char* s : lines) { g.text(s, 90.f, y, 16); y += 30.f; }
    g.button(backButton(), "BACK (Esc)");
}

void drawOverlay(Gfx& g, Screen s, const Game& w) {
    if (s == Screen::Playing) return;
    g.beginUi();
    g.rect(0, 0, VIEW_W, UI_H, Col(0, 0, 0, 170));
    std::string title = s == Screen::Paused ? "PAUSED" : s == Screen::MissionClear ? "MISSION COMPLETE"
                      : s == Screen::Won ? "CAMPAIGN COMPLETE" : "GAME OVER";
    Col tc = (s == Screen::Won || s == Screen::MissionClear) ? Col(80, 230, 120) : (s == Screen::Lost ? Col(230, 70, 70) : Col::White);
    g.text(title, VIEW_W / 2.f, 220.f, 52, tc, 1);

    if (s == Screen::Paused) {
        g.text("ESC or P = resume   |   F5 = save", VIEW_W / 2.f, 300.f, 22, Col(210, 210, 210), 1);
        return;
    }
    g.text("Score: " + std::to_string(w.score) + "   Kills: " + std::to_string(w.kills) + "   Time: " + mmss((int)w.gameTime),
           VIEW_W / 2.f, 300.f, 22, Col::White, 1);
    int done = 0;
    for (int i = 0; i < 3; ++i) if (w.objectivesDone & (1u << i)) ++done;
    g.text("Bonus objectives: " + std::to_string(done) + "/3   Difficulty: " + diffName(w.difficulty), VIEW_W / 2.f, 335.f, 18, Col(210, 210, 210), 1);

    if (s == Screen::MissionClear) {
        const MissionDef& next = MissionManager::defOf(w.mission.index() + 1);
        g.text("NEXT: MISSION " + std::to_string(w.mission.index() + 2) + " - " + next.title, VIEW_W / 2.f, 380.f, 22, Col(230, 170, 40), 1);
        g.text(next.brief, VIEW_W / 2.f, 412.f, 16, Col(200, 200, 200), 1);
        g.text("ENTER = next mission   |   ESC = main menu", VIEW_W / 2.f, 460.f, 20, Col(210, 210, 210), 1);
    } else {
        g.text("ENTER = play again   |   ESC = main menu", VIEW_W / 2.f, 390.f, 20, Col(210, 210, 210), 1);
    }
}

void drawToast(Gfx& g, const std::string& message) {
    g.beginUi();
    g.rect(VIEW_W / 2.f - 160.f, 86.f, 320.f, 30.f, Col(20, 24, 28, 230), Col(230, 170, 40), 1.f);
    g.text(message, VIEW_W / 2.f, 92.f, 16, Col::White, 1);
}
