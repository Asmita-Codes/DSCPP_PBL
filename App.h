#ifndef APP_H
#define APP_H

#include <string>
#include "Art.h"
#include "Audio.h"
#include "Fx.h"
#include "Game.h"
#include "Gfx.h"
#include "Leaderboard.h"
#include "Renderer.h"
#include "Screens.h"

class App {
public:
    explicit App(bool fullscreen);
    int run();                                   

private:
    void handleEvents();
    void onKey(SDL_Scancode key);
    void onLeftClick(FPt m);
    void update(float dt);
    void draw();

    void startNew(bool coop = false);           
    void beginMission();                         
    void doSave();
    bool doLoad();
    void finishRun();
    void toast(const std::string& s) { toastText_ = s; toastTimer_ = 2.f; }
    void playCues();                             
    void ambientSounds(float dt);                
    void nextSong();                             
    void toggleEffects();                    
    int hoveredButton() const;

    Gfx gfx_;
    Audio audio_;                                
    Art art_;
    Fx fx_;
    WorldRenderer world_;
    Game game_;
    Leaderboard board_;
    Screen screen_ = Screen::Menu;
    Difficulty difficulty_ = Difficulty::Normal;
    bool ok_ = false, running_ = true, debugPaths_ = false, runRecorded_ = false, gameInProgress_ = false;
    int cursorShown_ = -1;
    std::string toastText_;
    float toastTimer_ = 0.f;
    float emptyTimer_ = 0.f, heartTimer_ = 0.f, groanTimer_ = 2.f;
    int hover_ = -1;
    
    Input playerTwoInput(float dt);
    void pollGamepad(float dt);
    SDL_GameController* pad_ = nullptr;
    float padRetry_ = 0.f;
    bool padPrev_[SDL_CONTROLLER_BUTTON_MAX] = {};
    Vec2 p2Facing_ = Vec2(1.f, 0.f);
    std::mt19937 rng_{4242u};
};

#endif
