#include "App.h"
#include <algorithm>
#include <optional>
#include "GameSave.h"

namespace {
const char* const SAVE_FILE = "savegame.txt";
const char* const BOARD_FILE = "leaderboard.txt";
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
        toastTimer_ -= dt;
        draw();
    }
}

void App::startNew() {
    game_.newGame(difficulty_);
    runRecorded_ = false;
    screen_ = Screen::Playing;
}

void App::doSave() { toast(saveGameToFile(game_.toSave(), SAVE_FILE) ? "Game saved" : "Save failed"); }

bool App::doLoad() {
    SaveData d;
    if (!loadGameFromFile(d, SAVE_FILE)) { toast("No valid save file"); return false; }
    game_.fromSave(d);
    difficulty_ = d.difficulty;
    runRecorded_ = false;
    toast("Game loaded");
    return true;
}


void App::finishRun() {
    if (!runRecorded_) {
        board_.add("Player", game_.score, (int)game_.gameTime, (int)game_.difficulty);
        board_.save(BOARD_FILE);
        runRecorded_ = true;
    }
    screen_ = game_.status == Status::Won ? Screen::Won : Screen::Lost;
}

void App::handleEvents() {
    gfx_.worldView.setCenter(cameraCenter(game_));        
    while (const std::optional ev = gfx_.window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) gfx_.window.close();
        if (ev->is<sf::Event::Resized>()) gfx_.updateViewports();
        if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) onKey(k->code);
        if (const auto* mb = ev->getIf<sf::Event::MouseButtonPressed>()) {
            SVec m = gfx_.toUi(mb->position);
            if (mb->button == sf::Mouse::Button::Left) onLeftClick(m);
            if (mb->button == sf::Mouse::Button::Right && screen_ == Screen::Playing && m.x < VIEW_W) {
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
        if (key == K::Enter) { game_.nextMission(); screen_ = Screen::Playing; }
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
        else if (menuButton(1).contains(m)) { if (doLoad()) screen_ = Screen::Playing; }
        else if (menuButton(2).contains(m)) difficulty_ = (Difficulty)(((int)difficulty_ + 1) % 3);
        else if (menuButton(3).contains(m)) screen_ = Screen::Leaderboard;
        else if (menuButton(4).contains(m)) screen_ = Screen::Help;
        else if (menuButton(5).contains(m)) gfx_.createWindow(!gfx_.isFullscreen());
        else if (menuButton(6).contains(m)) gfx_.window.close();
    } else if (screen_ == Screen::Leaderboard || screen_ == Screen::Help) {
        if (backButton().contains(m)) screen_ = Screen::Menu;
    } else if (screen_ == Screen::Playing || screen_ == Screen::Paused) {
        if (sideButton(0).contains(m)) screen_ = screen_ == Screen::Playing ? Screen::Paused : Screen::Playing;
        else if (sideButton(1).contains(m)) doSave();
        else if (sideButton(2).contains(m)) doLoad();
        else if (sideButton(3).contains(m)) startNew();
        else if (sideButton(4).contains(m)) screen_ = Screen::Menu;
    }
}

void App::update(float dt) {
    gfx_.worldView.setCenter(cameraCenter(game_));
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
    in.fire = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && gfx_.mouseUi().x < VIEW_W;

    game_.update(dt, in);

    if (game_.status == Status::MissionClear) screen_ = Screen::MissionClear;
    else if (game_.status != Status::Playing) finishRun();
}

void App::draw() {
    gfx_.window.clear(Col(0, 0, 0));                 // black bars when the window is not 16:9
    gfx_.beginUi();
    gfx_.rect(0, 0, UI_W, UI_H, Col(12, 14, 16));
    switch (screen_) {
    case Screen::Menu: drawMenu(gfx_, saveFileExists(SAVE_FILE), difficulty_); break;
    case Screen::Leaderboard: drawLeaderboardScreen(gfx_, board_); break;
    case Screen::Help: drawHelp(gfx_); break;
    default:
        drawWorld(gfx_, game_, debugPaths_);
        drawHud(gfx_, game_, minimap_);
        drawOverlay(gfx_, screen_, game_);
        drawSidebar(gfx_, game_, board_, screen_ == Screen::Paused);
        break;
    }
    if (toastTimer_ > 0.f) drawToast(gfx_, toastText_);
    gfx_.window.display();
}
