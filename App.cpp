#include "App.h"
#include <algorithm>
#include <cmath>
#include "GameSave.h"

namespace {
const char* const SAVE_FILE = "savegame.txt";
const char* const BOARD_FILE = "leaderboard.txt";
}  

App::App(bool fullscreen) {
    if (!gfx_.init(fullscreen)) return;
   
    gfx_.beginFrame();
    gfx_.rectGrad(0, 0, UI_W, UI_H, Col(14, 18, 26), Col(4, 6, 10));
    gfx_.text("Building the city...", UI_W / 2, UI_H / 2 - 12, 20, C::Dim, Align::Center, Font::Title);
    gfx_.endFrame();

    if (!art_.build(gfx_) || !world_.init(gfx_)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Graphics error", SDL_GetError(), gfx_.win);
        return;
    }
    audio_.init();                         
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);   
    if (!board_.load(BOARD_FILE)) board_.seedSamples();
    game_.newGame(difficulty_);            
    beginMission();
    ok_ = true;
}

int App::run() {
    if (!ok_) return 1;
    Uint64 last = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();
    while (running_) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)std::min((now - last) / freq, 0.05);      
        last = now;
        handleEvents();
        {   
            bool overlay = screen_ == Screen::MissionClear || screen_ == Screen::Won || screen_ == Screen::Lost;
            audio_.setMusic(screen_ == Screen::Playing ? 1.f : screen_ == Screen::Paused ? 0.7f : overlay ? 0.3f : 0.75f,
                            screen_ == Screen::Paused);
        }
        int h = hoveredButton();                                    
        if (h != hover_ && h >= 0) audio_.play(Sfx::UiHover);
        hover_ = h;
        if (screen_ == Screen::Playing) update(dt);
        else if (screen_ == Screen::MissionClear || screen_ == Screen::Won || screen_ == Screen::Lost) fx_.update(game_, dt, false);
        toastTimer_ -= dt;
        draw();
        double spent = (SDL_GetPerformanceCounter() - now) / freq;   
        if (spent < 0.004) SDL_Delay(2);
    }
    return 0;
}

void App::beginMission() {
    fx_.reset(game_);
    world_.resetCamera(game_);
}

void App::startNew(bool coop) {
    game_.newGame(difficulty_, coop);
    runRecorded_ = false;
    gameInProgress_ = true;
    beginMission();
    screen_ = Screen::Playing;
}

void App::doSave() { toast(saveGameToFile(game_.toSave(), SAVE_FILE) ? "Game saved" : "Save failed"); }

bool App::doLoad() {
    SaveData d;
    if (!loadGameFromFile(d, SAVE_FILE)) { toast("No valid save file"); return false; }
    game_.fromSave(d);
    difficulty_ = d.difficulty;
    runRecorded_ = false;
    gameInProgress_ = true;
    beginMission();
    toast("Game loaded");
    return true;
}


void App::finishRun() {
    if (!runRecorded_) {
        board_.add(game_.coop ? "Co-op Team" : "Player", game_.score, (int)game_.gameTime, (int)game_.difficulty);
        board_.save(BOARD_FILE);
        runRecorded_ = true;
    }
    gameInProgress_ = false;        
    screen_ = game_.status == Status::Won ? Screen::Won : Screen::Lost;
    audio_.play(screen_ == Screen::Won ? Sfx::Victory : Sfx::GameOver);
}

void App::handleEvents() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT: running_ = false; break;
        case SDL_KEYDOWN:
            if (!ev.key.repeat) onKey(ev.key.keysym.scancode);
            break;
        case SDL_MOUSEBUTTONDOWN: {
            FPt m = gfx_.mouseUi();                                 
            if (ev.button.button == SDL_BUTTON_LEFT) onLeftClick(m);
            if (ev.button.button == SDL_BUTTON_RIGHT && screen_ == Screen::Playing) game_.toggleBarricade(world_.toWorld(m));
            break;
        }
        case SDL_RENDER_DEVICE_RESET:                             
            gfx_.dropTextCache();
            art_.build(gfx_);
            world_.init(gfx_);
            break;
        default: break;
        }
    }
}

void App::onKey(SDL_Scancode key) {
    if (key == SDL_SCANCODE_F11) { gfx_.setFullscreen(!gfx_.isFullscreen()); return; }
    if (key == SDL_SCANCODE_F12) { gfx_.requestScreenshot(); return; }
    if (key == SDL_SCANCODE_N) { nextSong(); return; }
    if (key == SDL_SCANCODE_X) { toggleEffects(); return; }
    const bool enter = key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER;

    switch (screen_) {
    case Screen::Menu:
        if (enter) startNew();
        break;
    case Screen::Multiplayer:
        if (enter) startNew(true);
        if (key == SDL_SCANCODE_ESCAPE) screen_ = Screen::Menu;
        break;
    case Screen::Leaderboard:
    case Screen::Help:
        if (key == SDL_SCANCODE_ESCAPE) screen_ = Screen::Menu;
        break;
    case Screen::MissionClear:
        if (enter) { game_.nextMission(); beginMission(); screen_ = Screen::Playing; }
        if (key == SDL_SCANCODE_ESCAPE) screen_ = Screen::Menu;
        break;
    case Screen::Won:
    case Screen::Lost:
        if (enter) startNew(game_.coop);                         
        if (key == SDL_SCANCODE_ESCAPE) screen_ = Screen::Menu;
        break;
    case Screen::Paused:
        if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_P) screen_ = Screen::Playing;
        if ((key == SDL_SCANCODE_F5 || key == SDL_SCANCODE_F9) && game_.coop) toast("Save / load is for single player");
        else if (key == SDL_SCANCODE_F5) doSave();
        else if (key == SDL_SCANCODE_F9) doLoad();
        if (key == SDL_SCANCODE_M) screen_ = Screen::Menu;      
        break;
    case Screen::Playing:
        if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_P) screen_ = Screen::Paused;
        if (key == SDL_SCANCODE_1) game_.weapon = 0;
        if (key == SDL_SCANCODE_2) game_.weapon = 1;
        if (key == SDL_SCANCODE_3 && !game_.useMedkit()) toast(game_.hp >= 100.f ? "Health is full" : "No medkits left");
        if (key == SDL_SCANCODE_5 || key == SDL_SCANCODE_F) game_.flashlight = !game_.flashlight;
        if (key == SDL_SCANCODE_E && !game_.throwGrenade(world_.toWorld(gfx_.mouseUi()))) toast("No grenades left");
        if (key == SDL_SCANCODE_U && !game_.undoBarricade()) toast("Nothing to undo");
        if (key == SDL_SCANCODE_B) {
            game_.useDijkstra = !game_.useDijkstra;
            game_.invalidatePaths();
            toast(game_.useDijkstra ? "Zombie AI: Dijkstra" : "Zombie AI: BFS");
        }
        if (key == SDL_SCANCODE_TAB) debugPaths_ = !debugPaths_;
        if ((key == SDL_SCANCODE_F5 || key == SDL_SCANCODE_F9) && game_.coop) toast("Save / load is for single player");
        else if (key == SDL_SCANCODE_F5) doSave();
        else if (key == SDL_SCANCODE_F9) doLoad();
        if (game_.coop) {                                            
            Vec2 front = game_.p2.pos + p2Facing_ * 36.f;
            if (key == SDL_SCANCODE_COMMA || key == SDL_SCANCODE_KP_3) game_.p2.weapon = 1 - game_.p2.weapon;
            if ((key == SDL_SCANCODE_PERIOD || key == SDL_SCANCODE_KP_2) && !game_.useMedkitP2() && game_.p2Up())
                toast(game_.p2.hp >= 100.f ? "P2: health is full" : "P2: no medkits left");
            if ((key == SDL_SCANCODE_SLASH || key == SDL_SCANCODE_KP_1) && game_.p2Up() &&
                !game_.throwGrenadeFrom(game_.p2.pos, game_.p2.pos + game_.p2.aimDir * 220.f)) toast("No grenades left");
            if ((key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_KP_ENTER) && game_.p2Up()) game_.toggleBarricade(front);
        }
        break;
    }
}

void App::onLeftClick(FPt m) {
    if (hoveredButton() >= 0) audio_.play(Sfx::UiClick);
    if (screen_ == Screen::Menu) {
        if (menuButton(0).contains(m.x, m.y)) startNew();
        else if (menuButton(1).contains(m.x, m.y)) screen_ = Screen::Multiplayer;
        else if (menuButton(2).contains(m.x, m.y)) {
            if (gameInProgress_) screen_ = Screen::Paused;          
            else if (doLoad()) screen_ = Screen::Playing;          
        }
        else if (menuButton(3).contains(m.x, m.y)) difficulty_ = (Difficulty)(((int)difficulty_ + 1) % 3);
        else if (menuButton(4).contains(m.x, m.y)) screen_ = Screen::Leaderboard;
        else if (menuButton(5).contains(m.x, m.y)) screen_ = Screen::Help;
        else if (menuButton(6).contains(m.x, m.y)) gfx_.setFullscreen(!gfx_.isFullscreen());
        else if (menuButton(7).contains(m.x, m.y)) nextSong();
        else if (menuButton(8).contains(m.x, m.y)) toggleEffects();
        else if (menuButton(9).contains(m.x, m.y)) running_ = false;
    } else if (screen_ == Screen::Multiplayer) {
        if (coopStartButton().contains(m.x, m.y)) startNew(true);
        else if (coopBackButton().contains(m.x, m.y)) screen_ = Screen::Menu;
    } else if (screen_ == Screen::Leaderboard || screen_ == Screen::Help) {
        if (backButton().contains(m.x, m.y)) screen_ = Screen::Menu;
    }
}


void App::update(float dt) {
    Input in;
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    const bool arrows = !game_.coop;                                 
    if (k[SDL_SCANCODE_W] || (arrows && k[SDL_SCANCODE_UP])) in.move.y -= 1.f;
    if (k[SDL_SCANCODE_S] || (arrows && k[SDL_SCANCODE_DOWN])) in.move.y += 1.f;
    if (k[SDL_SCANCODE_A] || (arrows && k[SDL_SCANCODE_LEFT])) in.move.x -= 1.f;
    if (k[SDL_SCANCODE_D] || (arrows && k[SDL_SCANCODE_RIGHT])) in.move.x += 1.f;
    in.sprint = k[SDL_SCANCODE_LSHIFT] || (arrows && k[SDL_SCANCODE_RSHIFT]);

    Vec2 aim = world_.toWorld(gfx_.mouseUi()) - game_.playerPos;
    in.aim = aim.length() > 1.f ? aim / aim.length() : game_.aimDir;
    in.fire = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;

    emptyTimer_ -= dt;                                               
    if (in.fire && emptyTimer_ <= 0.f && game_.inventory.count(game_.weapon == 0 ? "Rifle" : "Pistol") == 0) {
        audio_.play(Sfx::Empty);
        emptyTimer_ = 0.35f;
    }

    if (game_.coop) {
        Input in2 = playerTwoInput(dt);
        game_.update(dt, in, in2);
    } else {
        game_.update(dt, in);
    }
    fx_.update(game_, dt, true);
    world_.updateCamera(game_, dt);
    playCues();
    ambientSounds(dt);

    if (game_.status == Status::MissionClear) { screen_ = Screen::MissionClear; audio_.play(Sfx::MissionClear); }
    else if (game_.status != Status::Playing) finishRun();
}


void App::draw() {
    int wantCursor = screen_ == Screen::Playing ? SDL_DISABLE : SDL_ENABLE;   
    if (wantCursor != cursorShown_) { SDL_ShowCursor(wantCursor); cursorShown_ = wantCursor; }

    gfx_.beginFrame();
    switch (screen_) {
    case Screen::Menu:
        world_.drawBackdrop(gfx_, art_);
        drawMenu(gfx_, art_, gameInProgress_, saveFileExists(SAVE_FILE), difficulty_,
                 audio_.ok() ? std::string("MUSIC:  ") + musicTitle(audio_.track()) : std::string("NO SOUND DEVICE"),
                 audio_.ok() && audio_.effects());
        break;
    case Screen::Multiplayer:
        world_.drawBackdrop(gfx_, art_);
        drawMultiplayer(gfx_, art_, pad_ != nullptr, difficulty_);
        break;
    case Screen::Leaderboard:
        world_.drawBackdrop(gfx_, art_);
        drawLeaderboardScreen(gfx_, board_);
        break;
    case Screen::Help:
        world_.drawBackdrop(gfx_, art_);
        drawHelp(gfx_, art_);
        break;
    default:
        world_.draw(gfx_, art_, game_, fx_, debugPaths_);
        drawHud(gfx_, art_, world_, game_, fx_);             
        drawOverlay(gfx_, screen_, game_);
        if (screen_ == Screen::Playing) drawCrosshair(gfx_, fx_);
        break;
    }
    if (toastTimer_ > 0.f) drawToast(gfx_, toastText_, toastTimer_);
    gfx_.endFrame();
    if (!gfx_.lastScreenshot.empty()) { toast("Saved " + gfx_.lastScreenshot); gfx_.lastScreenshot.clear(); }
}

void App::playCues() {
    for (const SoundCue& c : fx_.cues) audio_.playAt(c.sfx, c.pos, game_.playerPos, c.vol, c.pitch);
    fx_.cues.clear();
}

void App::ambientSounds(float dt) {
    if (game_.hp > 0.f && game_.hp < 30.f) {                        
        heartTimer_ -= dt;
        if (heartTimer_ <= 0.f) { audio_.play(Sfx::Heartbeat, 0.6f + (30.f - game_.hp) / 60.f); heartTimer_ = 0.5f + game_.hp / 30.f * 0.45f; }
    }
    groanTimer_ -= dt;                                               
    if (groanTimer_ <= 0.f && !game_.zombies.empty()) {
        std::uniform_real_distribution<float> u(0.f, 1.f);
        groanTimer_ = 0.8f + u(rng_) * (game_.zombies.size() > 10 ? 1.2f : 3.f);
        const Zombie& z = game_.zombies[rng_() % game_.zombies.size()];
        float pitch = z.kind == ZombieKind::Tank ? 0.75f : z.kind == ZombieKind::Runner ? 1.3f : z.kind == ZombieKind::Spitter ? 1.12f : 1.f;
        if (z.kind == ZombieKind::Boss) audio_.playAt(Sfx::Roar, z.pos, game_.playerPos, 0.7f);
        else audio_.playAt(Sfx::Groan, z.pos, game_.playerPos, 0.85f, pitch);
    }
}

void App::nextSong() {
    if (!audio_.ok()) { toast("No sound device found"); return; }
    int t = audio_.track() + 1;                                    
    audio_.setTrack(t >= MUSIC_TRACKS ? -1 : t);
    toast(audio_.track() < 0 ? std::string("Music OFF") : std::string("Now playing:  ") + musicTitle(audio_.track()));
}

void App::toggleEffects() {
    if (!audio_.ok()) { toast("No sound device found"); return; }
    audio_.setEffects(!audio_.effects());
    toast(audio_.effects() ? "Sound effects ON" : "Sound effects OFF");
}

int App::hoveredButton() const {
    FPt m = gfx_.mouseUi();
    if (screen_ == Screen::Menu) {
        for (int i = 0; i < 10; ++i) if (menuButton(i).contains(m.x, m.y)) return i;
    } else if (screen_ == Screen::Multiplayer) {
        if (coopStartButton().contains(m.x, m.y)) return 200;
        if (coopBackButton().contains(m.x, m.y)) return 201;
    } else if (screen_ == Screen::Leaderboard || screen_ == Screen::Help) {
        if (backButton().contains(m.x, m.y)) return 100;
    }
    return -1;
}


void App::pollGamepad(float dt) {
    if (pad_ && !SDL_GameControllerGetAttached(pad_)) { SDL_GameControllerClose(pad_); pad_ = nullptr; }
    if (!pad_) {
        padRetry_ -= dt;
        if (padRetry_ > 0.f) return;
        padRetry_ = 1.f;                                             
        for (int i = 0; i < SDL_NumJoysticks(); ++i)
            if (SDL_IsGameController(i) && (pad_ = SDL_GameControllerOpen(i)) != nullptr) { toast("Gamepad connected: Player 2"); break; }
    }
}


Input App::playerTwoInput(float dt) {
    Input in2;
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    if (k[SDL_SCANCODE_UP]) in2.move.y -= 1.f;
    if (k[SDL_SCANCODE_DOWN]) in2.move.y += 1.f;
    if (k[SDL_SCANCODE_LEFT]) in2.move.x -= 1.f;
    if (k[SDL_SCANCODE_RIGHT]) in2.move.x += 1.f;
    in2.sprint = k[SDL_SCANCODE_RSHIFT];
    in2.fire = k[SDL_SCANCODE_RCTRL] || k[SDL_SCANCODE_KP_0];

    bool stickAim = false;
    Vec2 aim;
    pollGamepad(dt);
    if (pad_) {
        auto axis = [&](SDL_GameControllerAxis a) { return SDL_GameControllerGetAxis(pad_, a) / 32767.f; };
        Vec2 ls(axis(SDL_CONTROLLER_AXIS_LEFTX), axis(SDL_CONTROLLER_AXIS_LEFTY));
        Vec2 rs(axis(SDL_CONTROLLER_AXIS_RIGHTX), axis(SDL_CONTROLLER_AXIS_RIGHTY));
        if (ls.length() > 0.25f) in2.move = in2.move + ls;
        if (rs.length() > 0.35f) { aim = rs / rs.length(); stickAim = true; }
        if (axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 0.4f || SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_A)) in2.fire = true;
        if (SDL_GameControllerGetButton(pad_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) || axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 0.4f) in2.sprint = true;
        // buttons: act once per press
        auto pressed = [&](SDL_GameControllerButton b) {
            bool now = SDL_GameControllerGetButton(pad_, b) != 0, was = padPrev_[b];
            padPrev_[b] = now;
            return now && !was;
        };
        if (pressed(SDL_CONTROLLER_BUTTON_Y)) game_.p2.weapon = 1 - game_.p2.weapon;
        if (pressed(SDL_CONTROLLER_BUTTON_X)) game_.useMedkitP2();
        if (pressed(SDL_CONTROLLER_BUTTON_B) && game_.p2Up()) game_.throwGrenadeFrom(game_.p2.pos, game_.p2.pos + game_.p2.aimDir * 220.f);
        if (pressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) && game_.p2Up()) game_.toggleBarricade(game_.p2.pos + p2Facing_ * 36.f);
        if (pressed(SDL_CONTROLLER_BUTTON_START)) screen_ = Screen::Paused;
    }
    float ml = in2.move.length();
    if (ml > 0.1f) p2Facing_ = in2.move / ml;
    if (stickAim) { in2.aim = aim; p2Facing_ = aim; return in2; }

    in2.aim = p2Facing_;                                            
    float best = 480.f;
    for (const Zombie& z : game_.zombies) {
        Vec2 d = z.pos - game_.p2.pos;
        float l = d.length();
        if (l < 1.f || l >= best) continue;
        if ((d.x * p2Facing_.x + d.y * p2Facing_.y) / l < 0.34f) continue;   
        best = l;
        in2.aim = d / l;
    }
    return in2;
}
