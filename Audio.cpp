#include "Audio.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float TAU = 6.2831853f;

const char* const kNames[(int)Sfx::Count] = {
    "rifle", "pistol", "empty", "switch", "torch", "ricochet", "tinkle", "explosion", "throw",
    "groan", "roar", "zombie_hit", "zombie_death", "hurt", "heartbeat", "footstep",
    "pickup", "fuel", "medkit", "barricade_place", "barricade_break", "chew", "acid_spit", "acid_splash",
    "wave", "objective", "unlock", "mission_clear", "victory", "game_over", "ui_click", "ui_hover"};


const float kGain[(int)Sfx::Count] = {
    0.50f, 0.60f, 0.35f, 0.40f, 0.30f, 0.22f, 0.14f, 1.00f, 0.35f,
    0.42f, 0.80f, 0.42f, 0.55f, 0.50f, 0.75f, 0.14f,
    0.42f, 0.55f, 0.45f, 0.55f, 0.70f, 0.28f, 0.40f, 0.38f,
    0.40f, 0.50f, 0.50f, 0.55f, 0.60f, 0.60f, 0.32f, 0.12f};


const int kMinGap[(int)Sfx::Count] = {0, 0, 120, 60, 60, 45, 35, 40, 60, 250, 400, 30, 40, 250, 200, 0,
                                      40, 40, 100, 60, 60, 110, 80, 60, 1000, 300, 300, 500, 500, 500, 30, 25};
const int kMaxSame = 6;


struct SVF {                                    
    float low = 0.f, band = 0.f;
    float lp = 0.f, bp = 0.f, hp = 0.f;
    void run(float x, float fc, float damp, int sr) {
        float f = std::min(1.2f, 2.f * std::sin(PI * std::min(fc, sr * 0.16f) / sr));
        low += f * band;
        float high = x - low - damp * band;
        band += f * high;
        lp = low; bp = band; hp = high;
    }
};
struct LP1 {
    float y = 0.f;
    float run(float x, float fc, int sr) { float a = 1.f - std::exp(-TAU * fc / sr); y += a * (x - y); return y; }
};
inline float sat(float x) { return std::tanh(x); }
inline float note(float semisFromA4) { return 440.f * std::pow(2.f, semisFromA4 / 12.f); }

struct Gen {
    int sr;
    std::mt19937 rng{2024u};
    std::uniform_real_distribution<float> u{-1.f, 1.f};
    float n() { return u(rng); }
    float r(float a, float b) { return a + (b - a) * (u(rng) * 0.5f + 0.5f); }
    std::vector<float> make(float secs) { return std::vector<float>((size_t)(secs * sr), 0.f); }
};

void finish(std::vector<float>& b, int sr, float peak = 0.9f) {
    float m = 0.f;
    for (float v : b) m = std::max(m, std::fabs(v));
    if (m > 1e-6f) for (float& v : b) v *= peak / m;
    int fi = std::min((int)b.size(), sr / 500), fo = std::min((int)b.size(), sr / 60);      
    for (int i = 0; i < fi; ++i) b[(size_t)i] *= (float)i / fi;
    for (int i = 0; i < fo; ++i) b[b.size() - 1 - (size_t)i] *= (float)i / fo;
}

float bell(float f, float t) {
    return std::sin(TAU * f * t) + 0.35f * std::sin(TAU * 2.01f * f * t) * std::exp(-t * 4.f) +
           0.15f * std::sin(TAU * 3.98f * f * t) * std::exp(-t * 9.f);
}

std::vector<float> chime(Gen& g, float secs, std::initializer_list<std::pair<float, float>> notes, float decay) {
    auto b = g.make(secs);
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, s = 0.f;
        for (auto& nt : notes) {
            float lt = t - nt.first;
            if (lt < 0.f) continue;
            s += bell(nt.second, lt) * std::exp(-lt * decay) * std::min(1.f, lt * 200.f);
        }
        b[i] = s;
    }
    return b;
}


std::vector<float> gunshot(Gen& g, bool pistol) {
    auto b = g.make(pistol ? 0.55f : 0.42f);
    SVF crack; LP1 body, tail;
    float ph = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        crack.run(x, 3200.f * std::exp(-t * 30.f) + 700.f, 0.8f, g.sr);
        float c = crack.bp * std::exp(-t * (pistol ? 45.f : 60.f));
        float bd = body.run(x, pistol ? 1500.f : 1100.f, g.sr) * std::exp(-t * (pistol ? 11.f : 15.f)) * 1.4f;
        ph += TAU * ((pistol ? 120.f : 150.f) * std::exp(-t * 18.f) + 45.f) / g.sr;
        float th = std::sin(ph) * std::exp(-t * (pistol ? 18.f : 26.f)) * (pistol ? 1.1f : 0.8f);
        float tl = tail.run(x, 450.f, g.sr) * std::exp(-t * 5.f) * 0.5f;      // street echo
        b[i] = sat((c * 2.2f + bd + th + tl) * 1.6f);
    }
    return b;
}

std::vector<float> click(Gen& g, float secs, float tone, float decay, float noiseAmt) {
    auto b = g.make(secs);
    SVF f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr;
        f.run(g.n(), tone * 1.6f, 0.3f, g.sr);
        b[i] = (std::sin(TAU * tone * t) * 0.6f + f.bp * noiseAmt) * std::exp(-t * decay);
    }
    return b;
}

std::vector<float> weaponSwitch(Gen& g) {                
    auto b = g.make(0.32f);
    SVF f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, s = 0.f;
        f.run(g.n(), 2600.f, 0.15f, g.sr);
        for (float st : {0.f, 0.13f}) {
            float lt = t - st;
            if (lt >= 0.f) s += (f.bp * 1.2f + std::sin(TAU * 1250.f * lt) * 0.5f) * std::exp(-lt * 70.f);
        }
        b[i] = s;
    }
    return b;
}

std::vector<float> ricochet(Gen& g) {
    auto b = g.make(0.5f);
    float ph = 0.f;
    float f0 = g.r(2600.f, 3600.f);
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr;
        ph += TAU * (f0 * std::exp(-t * 2.2f) + 900.f + 40.f * std::sin(TAU * 30.f * t)) / g.sr;
        b[i] = std::sin(ph) * std::exp(-t * 7.f) + g.n() * std::exp(-t * 120.f) * 0.8f;
    }
    return b;
}

std::vector<float> tinkle(Gen& g) {                      
    auto b = g.make(0.3f);
    float f1 = g.r(3000.f, 3600.f), f2 = f1 * 1.52f, f3 = f1 * 2.03f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, s = 0.f;
        for (float st : {0.f, 0.07f, 0.13f}) {
            float lt = t - st;
            if (lt < 0.f) continue;
            float k = st == 0.f ? 1.f : st < 0.1f ? 0.45f : 0.2f;
            s += k * (std::sin(TAU * f1 * lt) * std::exp(-lt * 40.f) + 0.5f * std::sin(TAU * f2 * lt) * std::exp(-lt * 55.f) +
                      0.3f * std::sin(TAU * f3 * lt) * std::exp(-lt * 70.f));
        }
        b[i] = s;
    }
    return b;
}

std::vector<float> explosion(Gen& g) {
    auto b = g.make(2.4f);
    SVF rumble; LP1 deep;
    float ph = 0.f, crackle = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        float env = std::min(1.f, t / 0.006f) * std::exp(-t * 2.f);
        rumble.run(x, 2600.f * std::exp(-t * 3.f) + 120.f, 0.9f, g.sr);
        float d = deep.run(x, 180.f, g.sr) * 3.f;
        ph += TAU * (75.f * std::exp(-t * 3.f) + 26.f) / g.sr;
        float boom = std::sin(ph) * std::exp(-t * 3.f) * 1.3f;
        if (g.r(0.f, 1.f) < 0.0025f * std::exp(-t * 1.5f)) crackle = g.r(0.5f, 1.f);
        crackle *= 0.995f;
        b[i] = sat((rumble.lp * env * 1.6f + d * env + boom + crackle * g.n() * 0.6f) * 1.8f);
    }
    return b;
}

std::vector<float> whoosh(Gen& g) {
    auto b = g.make(0.32f);
    SVF f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, k = t / 0.32f;
        f.run(g.n(), 300.f + 1600.f * std::sin(PI * k), 0.5f, g.sr);
        b[i] = f.bp * std::sin(PI * k);
    }
    return b;
}


std::vector<float> voice(Gen& g, float secs, float f0, float fall, float f1, float f2, float breath, float drive, float jaw) {
    auto b = g.make(secs);
    SVF a, c, br;
    float ph = 0.f;
    float vib = g.r(4.f, 6.5f);
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, k = t / secs;
        float f = f0 * (1.f - fall * k) * (1.f + 0.04f * std::sin(TAU * vib * t)) * (1.f + 0.02f * g.n());
        ph += f / g.sr;
        ph -= std::floor(ph);
        float src = 2.f * ph - 1.f;                       // raw "vocal cords" (saw)
        a.run(src, f1 * (1.f + 0.15f * std::sin(TAU * 1.3f * t)), 0.35f, g.sr);
        c.run(src, f2, 0.4f, g.sr);
        br.run(g.n(), 900.f, 0.6f, g.sr);
        float env = std::min(1.f, t / 0.12f) * std::min(1.f, (secs - t) / 0.3f);
        float am = 1.f - jaw * (0.5f + 0.5f * std::sin(TAU * 6.5f * t));
        b[i] = sat((a.bp * 1.4f + c.bp * 0.8f + br.bp * breath) * env * am * drive);
    }
    return b;
}

std::vector<float> fleshHit(Gen& g) {
    auto b = g.make(0.2f);
    LP1 l; SVF sq;
    float ph = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        ph += TAU * (95.f * std::exp(-t * 15.f) + 50.f) / g.sr;
        sq.run(x, 1500.f, 0.5f, g.sr);
        b[i] = l.run(x, 900.f, g.sr) * 2.f * std::exp(-t * 35.f) + std::sin(ph) * std::exp(-t * 30.f) + sq.bp * std::exp(-t * 22.f) * 0.5f;
    }
    return b;
}

std::vector<float> zombieDeath(Gen& g) {
    auto b = g.make(0.75f);
    SVF gl; LP1 th;
    float ph = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        gl.run(x, 550.f + 280.f * std::sin(TAU * 9.f * t), 0.25f, g.sr);
        float gurgle = gl.bp * (0.5f + 0.5f * std::sin(TAU * 23.f * t)) * std::exp(-t * 4.f);
        ph += TAU * (70.f * std::exp(-t * 8.f) + 40.f) / g.sr;
        float thud = std::sin(ph) * std::exp(-t * 14.f) + th.run(x, 400.f, g.sr) * 2.f * std::exp(-t * 25.f);
        b[i] = sat((gurgle * 1.6f + thud) * 1.3f);
    }
    return b;
}

std::vector<float> heartbeat(Gen& g) {
    auto b = g.make(0.6f);
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, s = 0.f;
        for (auto st : {std::make_pair(0.f, 1.f), std::make_pair(0.17f, 0.7f)}) {
            float lt = t - st.first;
            if (lt >= 0.f) s += st.second * std::sin(TAU * (55.f - 15.f * std::min(1.f, lt * 8.f)) * lt) * std::exp(-lt * 16.f) * std::min(1.f, lt * 300.f);
        }
        b[i] = s;
    }
    return b;
}

std::vector<float> footstep(Gen& g) {
    auto b = g.make(0.13f);
    LP1 l; SVF grit;
    float fc = g.r(300.f, 480.f);
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        grit.run(x, 3500.f, 0.7f, g.sr);
        b[i] = l.run(x, fc, g.sr) * 3.f * std::exp(-t * 40.f) + grit.hp * 0.25f * std::exp(-t * 60.f);
    }
    return b;
}

std::vector<float> woodThunk(Gen& g) {
    auto b = g.make(0.4f);
    SVF res;
    float ph = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        ph += TAU * (170.f * std::exp(-t * 6.f) + 100.f) / g.sr;
        res.run(x * std::exp(-t * 60.f), 420.f, 0.08f, g.sr);
        b[i] = std::sin(ph) * std::exp(-t * 20.f) + res.bp * 3.f + x * std::exp(-t * 300.f) * 0.6f;
    }
    return b;
}

std::vector<float> woodBreak(Gen& g) {
    auto b = g.make(1.0f);
    SVF c[5], rattle;
    float starts[5];
    for (float& s : starts) s = g.r(0.f, 0.28f);
    float ph = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n(), s = 0.f;
        for (int k = 0; k < 5; ++k) {
            float lt = t - starts[k];
            if (lt < 0.f) continue;
            c[k].run(x, 1400.f + 400.f * k, 0.3f, g.sr);
            s += c[k].bp * std::exp(-lt * 45.f) * 1.4f;
        }
        ph += TAU * 85.f / g.sr;
        rattle.run(x, 2500.f, 0.8f, g.sr);
        b[i] = sat(s + std::sin(ph) * std::exp(-t * 9.f) * 0.8f + rattle.hp * std::exp(-t * 4.f) * 0.25f);
    }
    return b;
}

std::vector<float> spit(Gen& g) {
    auto b = g.make(0.4f);
    SVF f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, k = t / 0.4f;
        f.run(g.n(), 400.f + 2200.f * k, 0.25f, g.sr);
        float blip = std::sin(TAU * (300.f + 900.f * k) * t) * (std::sin(TAU * 31.f * t) > 0.6f ? 0.5f : 0.f);
        b[i] = (f.bp * 1.4f + blip) * std::sin(PI * std::min(1.f, k * 1.3f)) * std::exp(-t * 3.f);
    }
    return b;
}

std::vector<float> sizzle(Gen& g) {
    auto b = g.make(0.55f);
    SVF f;
    float crack = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, x = g.n();
        f.run(x, 3800.f, 0.6f, g.sr);
        if (g.r(0.f, 1.f) < 0.004f) crack = 1.f;
        crack *= 0.993f;
        b[i] = (f.hp * (0.5f + crack) + g.n() * crack * 0.3f) * std::min(1.f, t * 100.f) * std::exp(-t * 5.f);
    }
    return b;
}

std::vector<float> medkit(Gen& g) {
    auto b = g.make(0.7f);
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, k = t / 0.7f;
        float f = 600.f + 700.f * k;
        float s = std::sin(TAU * f * t + 2.f * std::sin(TAU * 7.f * t)) + 0.5f * std::sin(TAU * f * 1.5f * t);
        b[i] = s * std::sin(PI * k) * 0.7f;
    }
    return b;
}


std::vector<float> siren(Gen& g) {                       
    auto b = g.make(2.0f);
    LP1 l;
    float p1 = 0.f, p2 = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr;
        float f = 200.f + 70.f * (0.5f - 0.5f * std::cos(TAU * 0.5f * t));
        p1 += f / g.sr; p2 += f * 0.501f / g.sr;
        p1 -= std::floor(p1); p2 -= std::floor(p2);
        float s = (2.f * p1 - 1.f) + 0.7f * (2.f * p2 - 1.f);
        float env = std::min(1.f, t / 0.25f) * std::min(1.f, (2.f - t) / 0.6f);
        b[i] = l.run(s, 1100.f, g.sr) * env;
    }
    return b;
}

std::vector<float> gameOver(Gen& g) {
    auto b = g.make(2.8f);
    LP1 l;
    const float notes[3] = {note(-12), note(-16), note(-19)};     
    float ph = 0.f;
    for (size_t i = 0; i < b.size(); ++i) {
        float t = (float)i / g.sr, s = 0.f;
        for (int k = 0; k < 3; ++k) {
            float lt = t - k * 0.45f;
            if (lt >= 0.f) s += bell(notes[k], lt) * std::exp(-lt * (k == 2 ? 1.4f : 3.f));
        }
        ph += note(-31) / g.sr; ph -= std::floor(ph);              // dark drone underneath
        s += l.run(2.f * ph - 1.f, 300.f, g.sr) * 0.6f * std::min(1.f, t * 2.f) * std::exp(-t * 0.7f);
        b[i] = s;
    }
    return b;
}

}  


Audio::~Audio() {
    quit_ = true;
    if (worker_) SDL_WaitThread(worker_, nullptr);       
    if (dev_) SDL_CloseAudioDevice(dev_);
}

int Audio::composer(void* p) {                           
    Audio* a = static_cast<Audio*>(p);
    while (!a->quit_) {
        int t = a->want_.load();
        if (t >= 0 && t < MUSIC_TRACKS && !a->ready_[t]) {
            a->tracks_[t] = composeMusic(a->sr_, t);
            a->ready_[t].store(1, std::memory_order_release);
        } else {
            SDL_Delay(30);
        }
    }
    return 0;
}

bool Audio::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = &Audio::callback;
    want.userdata = this;
    SDL_AudioDeviceID d = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (d == 0) return false;
    sr_ = have.freq;
    synthesize();                                       
    loadOverrides();
    voices_.reserve(64);
    dev_ = d;
    SDL_PauseAudioDevice(dev_, 0);
    worker_ = SDL_CreateThread(&Audio::composer, "music", this);
    if (!worker_) composer(this);                       
    return true;
}

void Audio::synthesize() {
    Gen g{sr_};
    auto put = [&](Sfx s, std::vector<float> b, float peak = 0.9f) { finish(b, sr_, peak); bank_[(int)s].push_back(std::move(b)); };
    put(Sfx::Rifle, gunshot(g, false));            put(Sfx::Rifle, gunshot(g, false));
    put(Sfx::Pistol, gunshot(g, true));
    put(Sfx::Empty, click(g, 0.07f, 1900.f, 140.f, 1.5f));
    put(Sfx::Switch, weaponSwitch(g));
    put(Sfx::Torch, click(g, 0.06f, 2400.f, 160.f, 1.f));
    put(Sfx::Ricochet, ricochet(g));               put(Sfx::Ricochet, ricochet(g));
    put(Sfx::Tinkle, tinkle(g));                   put(Sfx::Tinkle, tinkle(g));
    put(Sfx::Explosion, explosion(g));
    put(Sfx::Throw, whoosh(g));
    put(Sfx::Groan, voice(g, 1.1f, 92.f, 0.25f, 520.f, 1150.f, 0.35f, 1.8f, 0.35f));
    put(Sfx::Groan, voice(g, 0.9f, 110.f, 0.35f, 600.f, 1300.f, 0.45f, 2.0f, 0.25f));
    put(Sfx::Groan, voice(g, 1.3f, 80.f, -0.15f, 460.f, 1000.f, 0.3f, 1.6f, 0.45f));
    put(Sfx::Roar, voice(g, 1.8f, 52.f, 0.3f, 380.f, 820.f, 0.8f, 3.5f, 0.15f));
    put(Sfx::ZHit, fleshHit(g));                   put(Sfx::ZHit, fleshHit(g));
    put(Sfx::ZDeath, zombieDeath(g));
    put(Sfx::Hurt, voice(g, 0.32f, 220.f, 0.35f, 720.f, 1250.f, 0.4f, 1.6f, 0.f));
    put(Sfx::Heartbeat, heartbeat(g));
    put(Sfx::Footstep, footstep(g));               put(Sfx::Footstep, footstep(g));     put(Sfx::Footstep, footstep(g));
    put(Sfx::Pickup, chime(g, 0.5f, {{0.f, note(15)}, {0.07f, note(22)}}, 8.f));                       // C6 G6
    put(Sfx::Fuel, chime(g, 0.8f, {{0.f, note(15)}, {0.07f, note(19)}, {0.14f, note(22)}, {0.21f, note(27)}}, 6.f));
    put(Sfx::Medkit, medkit(g));
    put(Sfx::Place, woodThunk(g));
    put(Sfx::Break, woodBreak(g));
    put(Sfx::Chew, click(g, 0.12f, 260.f, 45.f, 2.f));  put(Sfx::Chew, click(g, 0.12f, 320.f, 50.f, 2.f));
    put(Sfx::Spit, spit(g));
    put(Sfx::Splash, sizzle(g));
    put(Sfx::Wave, siren(g));
    put(Sfx::Objective, chime(g, 0.7f, {{0.f, note(10)}, {0.09f, note(17)}}, 5.f));                    // G5 D6
    put(Sfx::Unlock, chime(g, 1.2f, {{0.f, note(3)}, {0.f, note(7)}, {0.f, note(10)}, {0.12f, note(15)}}, 2.5f));
    put(Sfx::MissionClear, chime(g, 1.8f, {{0.f, note(3)}, {0.12f, note(7)}, {0.24f, note(10)}, {0.36f, note(15)}, {0.36f, note(10)}, {0.36f, note(7)}}, 2.2f));
    put(Sfx::Victory, chime(g, 2.6f, {{0.f, note(-2)}, {0.15f, note(3)}, {0.3f, note(7)}, {0.45f, note(10)}, {0.7f, note(15)}, {0.7f, note(19)}, {0.7f, note(22)}, {0.7f, note(3)}}, 1.5f));
    put(Sfx::GameOver, gameOver(g));
    put(Sfx::UiClick, click(g, 0.06f, 1300.f, 90.f, 0.6f));
    put(Sfx::UiHover, click(g, 0.035f, 2100.f, 160.f, 0.2f));
}


bool Audio::loadWav(const char* path, std::vector<float>& out, int channels) {
    SDL_AudioSpec spec;
    Uint8* data = nullptr;
    Uint32 len = 0;
    if (!SDL_LoadWAV(path, &spec, &data, &len)) return false;
    SDL_AudioStream* st = SDL_NewAudioStream(spec.format, spec.channels, spec.freq, AUDIO_F32SYS, (Uint8)channels, sr_);
    bool ok = false;
    if (st && SDL_AudioStreamPut(st, data, (int)len) == 0 && SDL_AudioStreamFlush(st) == 0) {
        int avail = SDL_AudioStreamAvailable(st);
        out.assign((size_t)avail / sizeof(float), 0.f);
        ok = SDL_AudioStreamGet(st, out.data(), avail) > 0;
    }
    if (st) SDL_FreeAudioStream(st);
    SDL_FreeWAV(data);
    return ok && !out.empty();
}

void Audio::loadOverrides() {
    std::string base;
    if (char* b = SDL_GetBasePath()) { base = b; SDL_free(b); }
    for (int i = 0; i < (int)Sfx::Count; ++i) {
        std::vector<float> buf;
        std::string rel = std::string("assets/sfx/") + kNames[i] + ".wav";
        if (loadWav((base + rel).c_str(), buf) || loadWav(rel.c_str(), buf)) {
            finish(buf, sr_);
            bank_[i].clear();
            bank_[i].push_back(std::move(buf));
        }
    }
    for (int t = 0; t < MUSIC_TRACKS; ++t) {             
        std::string rel = t == 0 ? std::string("assets/sfx/music.wav") : "assets/sfx/music" + std::to_string(t + 1) + ".wav";
        std::vector<float> mus;
        if (loadWav((base + rel).c_str(), mus, 2) || loadWav(rel.c_str(), mus, 2)) {
            float m = 0.f;
            for (float v : mus) m = std::max(m, std::fabs(v));
            for (float& v : mus) v *= 0.9f / std::max(m, 1e-6f);
            tracks_[t] = std::move(mus);
            ready_[t] = 1;
        }
    }
}


void Audio::play(Sfx s, float vol, float pan, float pitch) {
    int id = (int)s;
    const bool jingle = s == Sfx::MissionClear || s == Sfx::Victory || s == Sfx::GameOver || s == Sfx::Objective || s == Sfx::Unlock;
    if (!dev_ || (!sfxOn_ && !jingle) || bank_[id].empty() || vol <= 0.01f) return;
    Uint32 now = SDL_GetTicks();
    if (lastPlay_[id] && now - lastPlay_[id] < (Uint32)kMinGap[id]) return;
    lastPlay_[id] = now;
    const std::vector<float>* buf = &bank_[id][rng_() % bank_[id].size()];
    pan = std::clamp(pan, -1.f, 1.f);
    float v = vol * kGain[id];
    Voice nv{buf, 0.0, pitch, v * std::min(1.f, 1.f - pan), v * std::min(1.f, 1.f + pan), id};
    SDL_LockAudioDevice(dev_);
    int same = 0;
    for (const Voice& o : voices_) if (o.sfx == id) ++same;
    if (same < kMaxSame) {
        if (voices_.size() >= 48) {                       
            size_t best = 0;
            double most = -1.0;
            for (size_t i = 0; i < voices_.size(); ++i) {
                double done = voices_[i].pos / (double)voices_[i].buf->size();
                if (done > most) { most = done; best = i; }
            }
            voices_[best] = nv;
        } else voices_.push_back(nv);
    }
    SDL_UnlockAudioDevice(dev_);
}

void Audio::playAt(Sfx s, Vec2 pos, Vec2 listener, float vol, float pitch) {
    Vec2 d = pos - listener;
    float dist = d.length();
    float att = std::pow(std::clamp(1.f - dist / 1100.f, 0.f, 1.f), 1.6f);
    if (att <= 0.01f) return;
    std::uniform_real_distribution<float> var(0.94f, 1.06f);
    play(s, vol * att, d.x / 700.f, pitch * var(rng_));
}

void Audio::setMusic(float level, bool muffled) {
    musicTarget_ = track_ >= 0 ? std::clamp(level, 0.f, 1.f) : 0.f;
    lpTarget_ = muffled ? 0.06f : 1.f;                   
}

void Audio::setTrack(int t) {
    track_ = (t >= 0 && t < MUSIC_TRACKS) ? t : -1;
    if (track_ < 0) { musicTarget_ = 0.f; return; }
    want_ = track_;                                      
    if (!dev_) return;
    SDL_LockAudioDevice(dev_);
    if (track_ != curTrack_) { curTrack_ = track_; musicPos_ = 0; musicGain_ = 0.f; }
    SDL_UnlockAudioDevice(dev_);
}

void Audio::setEffects(bool on) {
    sfxOn_ = on;
    if (!on && dev_) { SDL_LockAudioDevice(dev_); voices_.clear(); SDL_UnlockAudioDevice(dev_); }
}


void Audio::callback(void* self, Uint8* stream, int len) {
    static_cast<Audio*>(self)->mix(reinterpret_cast<float*>(stream), len / (int)(2 * sizeof(float)));
}

void Audio::mix(float* out, int frames) {
    std::fill(out, out + frames * 2, 0.f);
    for (size_t vi = 0; vi < voices_.size();) {
        Voice& v = voices_[vi];
        const std::vector<float>& b = *v.buf;
        const size_t n = b.size();
        int i = 0;
        for (; i < frames; ++i) {
            size_t idx = (size_t)v.pos;
            if (idx + 1 >= n) break;
            float fr = (float)(v.pos - (double)idx);
            float s = b[idx] + (b[idx + 1] - b[idx]) * fr;
            out[2 * i] += s * v.gl;
            out[2 * i + 1] += s * v.gr;
            v.pos += v.rate;
        }
        if (i < frames) { voices_[vi] = voices_.back(); voices_.pop_back(); }
        else ++vi;
    }
    const std::vector<float>* mus = ready_[curTrack_].load(std::memory_order_acquire) ? &tracks_[curTrack_] : nullptr;
    const std::vector<float>& music = mus ? *mus : tracks_[curTrack_];
    const size_t mframes = mus ? mus->size() / 2 : 0;
    if (!mus) musicGain_ = 0.f;                                  
    for (int i = 0; i < frames; ++i) {
        musicGain_ += (musicTarget_ - musicGain_) * 0.00004f;     
        lpCoef_ += (lpTarget_ - lpCoef_) * 0.00006f;
        master_ += (masterTarget_ - master_) * 0.0005f;
        float l = out[2 * i], r = out[2 * i + 1];
        if (mframes) {                                         
            lpL_ += lpCoef_ * (music[2 * musicPos_] - lpL_);
            lpR_ += lpCoef_ * (music[2 * musicPos_ + 1] - lpR_);
            float k = musicGain_ * 0.55f * (1.f + (1.f - lpCoef_) * 0.6f);  
            l += lpL_ * k;
            r += lpR_ * k;
            if (++musicPos_ >= mframes) musicPos_ = 0;
        }
        l *= master_; r *= master_;
        l = std::clamp(l, -3.f, 3.f); r = std::clamp(r, -3.f, 3.f);       
        out[2 * i] = l * (27.f + l * l) / (27.f + 9.f * l * l);
        out[2 * i + 1] = r * (27.f + r * r) / (27.f + 9.f * r * r);
    }
}
