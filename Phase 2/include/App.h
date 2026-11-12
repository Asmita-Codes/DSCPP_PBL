#ifndef APP_H
#define APP_H

#include <string>
#include "Game.h"
#include "Gfx.h"
#include "Leaderboard.h"
#include "Renderer.h"
#include "Screens.h"

class App {
public:
    explicit App(bool fullscreen);
    void run();

private:
    void handleEvents();
    void onKey(sf::Keyboard::Key key);
    void onLeftClick(SVec m);
    void update(float dt);
    void draw();

    void startNew();
    void doSave();
    bool doLoad();
    void finishRun();
    void toast(const std::string& s) { toastText_ = s; toastTimer_ = 2.f; }

    Gfx gfx_;
    Game game_;
    Leaderboard board_;
    Minimap minimap_;
    Screen screen_ = Screen::Menu;
    Difficulty difficulty_ = Difficulty::Normal;
    bool debugPaths_ = false, runRecorded_ = false;
    std::string toastText_;
    float toastTimer_ = 0.f;
};

#endif
