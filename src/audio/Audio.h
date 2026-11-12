
#ifndef AUDIO_H
#define AUDIO_H

#include <atomic>
#include <random>
#include <vector>
#include "Music.h"
#include "GameTypes.h"
#include "SdlInc.h"

enum class Sfx {
    Rifle, Pistol, Empty, Switch, Torch, Ricochet, Tinkle, Explosion, Throw,
    Groan, Roar, ZHit, ZDeath, Hurt, Heartbeat, Footstep,
    Pickup, Fuel, Medkit, Place, Break, Chew, Spit, Splash,
    Wave, Objective, Unlock, MissionClear, Victory, GameOver, UiClick, UiHover,
    Count
};

class Audio {
public:
    ~Audio();
    bool init();                                        
    bool ok() const { return dev_ != 0; }

    void play(Sfx s, float vol = 1.f, float pan = 0.f, float pitch = 1.f);
    void playAt(Sfx s, Vec2 pos, Vec2 listener, float vol = 1.f, float pitch = 1.f);   
    void setMusic(float level, bool muffled);          
    void setTrack(int t);                             
    int track() const { return track_; }
    void setEffects(bool on);                           
    bool effects() const { return sfxOn_; }

private:
    struct Voice { const std::vector<float>* buf; double pos; float rate, gl, gr; int sfx; };
    static void callback(void* self, Uint8* stream, int len);
    void mix(float* out, int frames);
    void synthesize();
    void loadOverrides();
    bool loadWav(const char* path, std::vector<float>& out, int channels = 1);
    static int composer(void* self);                     

    SDL_AudioDeviceID dev_ = 0;
    int sr_ = 44100;
    int track_ = 0;
    bool sfxOn_ = false;                                
    std::vector<std::vector<float>> bank_[(int)Sfx::Count];   
    std::vector<float> tracks_[MUSIC_TRACKS];           
    std::atomic<int> ready_[MUSIC_TRACKS] = {};         
    std::atomic<bool> quit_{false};
    std::atomic<int> want_{0};                      
    SDL_Thread* worker_ = nullptr;
    int curTrack_ = 0;                                  
    std::vector<Voice> voices_;
    size_t musicPos_ = 0;                               
    float musicGain_ = 0.f, musicTarget_ = 0.f, lpCoef_ = 1.f, lpTarget_ = 1.f, lpL_ = 0.f, lpR_ = 0.f;
    float master_ = 0.f, masterTarget_ = 0.85f;
    Uint32 lastPlay_[(int)Sfx::Count] = {};
    std::mt19937 rng_{777u};
};

#endif
