#include "Music.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {
constexpr float TAU = 6.2831853f;
inline float mtof(float m) { return 440.f * std::pow(2.f, (m - 69.f) / 12.f); }

struct SVF {                                         
    float low = 0.f, band = 0.f, hp = 0.f;
    void run(float x, float fc, float damp, int sr) {
        float f = std::min(1.2f, 2.f * std::sin(3.14159265f * std::min(fc, sr * 0.16f) / sr));
        low += f * band;
        hp = x - low - damp * band;
        band += f * hp;
    }
};

struct Note { int step, midi, len; };
struct Studio {
    int sr = 44100;
    double step = 0.1;                               
    std::vector<float> drums, bass, chords, lead, pad, hats, arp;
    std::vector<double> kicks;                       
    std::mt19937 rng{1312u};
    std::uniform_real_distribution<float> u{-1.f, 1.f};
    float n() { return u(rng); }
    size_t at(double t) const { return (size_t)(t * sr); }
    double T(int bar, double st) const { return (bar * 16 + st) * step; }

    void kick(double t0, float vel, float clickAmt = 0.25f) {
        kicks.push_back(t0);
        size_t a = at(t0), len = at(0.32);
        float ph = 0.f;
        for (size_t i = 0; i < len && a + i < drums.size(); ++i) {
            float t = (float)i / sr;
            ph += TAU * (48.f + 110.f * std::exp(-t * 30.f)) / sr;
            float s = std::sin(ph) * std::exp(-t * 8.f) + n() * std::exp(-t * 400.f) * clickAmt;
            drums[a + i] += vel * std::tanh(1.5f * s);
        }
    }
    void snare(double t0, float vel) {
        size_t a = at(t0), len = at(0.2);
        SVF f;
        for (size_t i = 0; i < len && a + i < drums.size(); ++i) {
            float t = (float)i / sr;
            f.run(n(), 2200.f, 0.6f, sr);
            drums[a + i] += vel * (f.band * std::exp(-t * 24.f) * 1.2f + std::sin(TAU * 185.f * t) * std::exp(-t * 30.f) * 0.6f);
        }
    }
    void clap(double t0, float vel) {
        size_t a = at(t0), len = at(0.2);
        SVF f;
        for (size_t i = 0; i < len && a + i < drums.size(); ++i) {
            float t = (float)i / sr, env = 0.f;
            for (float b : {0.f, 0.009f, 0.018f}) if (t >= b) env += std::exp(-(t - b) * 180.f);
            if (t > 0.02f) env += std::exp(-(t - 0.02f) * 30.f) * 0.5f;
            f.run(n(), 1300.f, 0.5f, sr);
            drums[a + i] += vel * f.band * env * 1.6f;
        }
    }
    void hat(double t0, float vel) {                 
        size_t a = at(t0), len = at(0.04);
        SVF f;
        for (size_t i = 0; i < len && a + i < hats.size(); ++i) {
            float t = (float)i / sr;
            f.run(n(), 8000.f, 0.3f, sr);
            hats[a + i] += vel * f.hp * std::exp(-t * 90.f);
        }
    }
    
    void bassNote(double t0, float midi, double dur, bool warm) {
        size_t a = at(t0), len = at(dur + 0.03);
        float f = mtof(midi), ph = 0.f, y = 0.f;
        for (size_t i = 0; i < len && a + i < bass.size(); ++i) {
            float t = (float)i / sr;
            ph += f / sr; ph -= std::floor(ph);
            float env = std::min(1.f, t / 0.004f) * std::min(1.f, (float)(dur + 0.03 - t) / 0.03f);
            float s;
            if (warm) {                                                    
                float tri = 1.f - 4.f * std::fabs(ph - 0.5f);
                s = (std::sin(TAU * ph) * 0.8f + tri * 0.35f) * (0.75f + 0.25f * std::exp(-t * 10.f));
            } else {                                                       
                float saw = 2.f * ph - 1.f, sq = ph < 0.5f ? 1.f : -1.f;
                float fc = 180.f + 1400.f * std::exp(-t * 14.f);
                y += (1.f - std::exp(-TAU * fc / sr)) * ((saw * 0.6f + sq * 0.4f) - y);
                s = y + std::sin(TAU * ph) * 0.6f;
            }
            bass[a + i] += s * env;
        }
    }
    void pluck(std::vector<float>& bus, double t0, float midi, float vel, float ratio = 2.f, float index = 2.4f, float decay = 5.5f) {
        size_t a = at(t0), len = at(std::min(1.4f, 4.f / decay));
        float f = mtof(midi), cp = 0.f, mp = 0.f;
        for (size_t i = 0; i < len && a + i < bus.size(); ++i) {
            float t = (float)i / sr;
            cp += f / sr; mp += ratio * f / sr;
            float idx = index * std::exp(-t * 9.f);
            bus[a + i] += vel * std::sin(TAU * cp + idx * std::sin(TAU * mp)) * std::exp(-t * decay) * std::min(1.f, t * 500.f);
        }
    }
    void kalimba(double t0, float midi, float vel) {                    
        size_t a = at(t0), len = at(1.2);
        float f = mtof(midi);
        for (size_t i = 0; i < len && a + i < lead.size(); ++i) {
            float t = (float)i / sr;
            float s = std::sin(TAU * f * t) * std::exp(-t * 3.2f) + 0.35f * std::sin(TAU * 4.f * f * t) * std::exp(-t * 14.f)
                    + 0.12f * std::sin(TAU * 2.f * f * t) * std::exp(-t * 6.f);
            lead[a + i] += vel * s * std::min(1.f, t * 700.f);
        }
    }
    void flute(double t0, float midi, double dur, float vel) {          
        size_t a = at(t0), len = at(dur + 0.15);
        float f = mtof(midi), ph = 0.f;
        for (size_t i = 0; i < len && a + i < lead.size(); ++i) {
            float t = (float)i / sr;
            float vib = 1.f + (t > 0.15f ? 0.006f * std::sin(TAU * 5.f * t) : 0.f);
            ph += f * vib / sr; ph -= std::floor(ph);
            float env = std::min(1.f, t / 0.06f) * std::min(1.f, (float)(dur + 0.15 - t) / 0.15f);
            lead[a + i] += vel * (std::sin(TAU * ph) + 0.15f * std::sin(2.f * TAU * ph)) * env;
        }
    }
    void synthLead(double t0, float midi, double dur, float vel) {
        size_t a = at(t0), len = at(dur + 0.08);
        float f = mtof(midi), p1 = 0.f, p2 = 0.f, y = 0.f;
        for (size_t i = 0; i < len && a + i < lead.size(); ++i) {
            float t = (float)i / sr;
            float vib = t > 0.12f ? 0.012f * std::sin(TAU * 5.5f * t) : 0.f;
            p1 += f * (1.f + vib) / sr; p1 -= std::floor(p1);
            p2 += f * 1.005f * (1.f + vib) / sr; p2 -= std::floor(p2);
            float s = ((p1 < 0.35f ? 1.f : -1.f) + 0.3f) * 0.6f + (2.f * p2 - 1.f) * 0.5f;
            y += (1.f - std::exp(-TAU * 3200.f / sr)) * (s - y);
            float env = std::min(1.f, t / 0.006f) * std::min(1.f, (float)(dur + 0.08 - t) / 0.07f);
            lead[a + i] += vel * y * env;
        }
    }
    void epiano(double t0, float midi, double dur, float vel) {         
        size_t a = at(t0), len = at(dur + 0.25);
        const float f = mtof(midi), w = TAU * f / sr, relLen = 0.25f * sr;
        const float d3 = std::exp(-3.f / sr), d25 = std::exp(-25.f / sr), d13 = std::exp(-1.3f / sr);
        float e3 = 1.f, e25 = 1.f, e13 = 1.f;                            
        for (size_t i = 0; i < len && a + i < chords.size(); ++i) {
            float ph = w * (float)i;
            float s = std::sin(ph + (1.1f * e3 + 0.25f) * std::sin(ph)) + 0.12f * std::sin(14.f * ph) * e25;
            float env = e13 * std::min(1.f, (float)i / (sr * 0.0025f)) * std::min(1.f, (float)(len - i) / relLen);
            chords[a + i] += vel * s * env;
            e3 *= d3; e25 *= d25; e13 *= d13;
        }
    }
    void padChord(double t0, const int* notes, int count, double dur, float vel) {
        size_t a = at(t0), len = at(dur + 0.5);
        float ph[8] = {}, inc[8] = {}, y = 0.f;
        for (int k = 0; k < count * 2; ++k) inc[k] = mtof((float)notes[k / 2]) * (k % 2 ? 1.004f : 0.996f) / sr;   
        const float lpA = 1.f - std::exp(-TAU * 800.f / sr);
        for (size_t i = 0; i < len && a + i < pad.size(); ++i) {
            float t = (float)i / sr, s = 0.f;
            for (int k = 0; k < count * 2; ++k) {
                ph[k] += inc[k];
                ph[k] -= std::floor(ph[k]);
                s += 2.f * ph[k] - 1.f;
            }
            y += lpA * (s - y);
            float env = std::min(1.f, t / 0.5f) * std::min(1.f, (float)(dur + 0.5 - t) / 0.5f);
            pad[a + i] += vel * y * env;
        }
    }
    void string(std::vector<float>& bus, double t0, float midi, float vel, float decay) {   
        size_t a = at(t0), len = at(1.6);
        int period = std::max(2, (int)std::lround(sr / mtof(midi)));
        std::vector<float> d((size_t)period);
        float lp = 0.f;
        for (float& v : d) { lp += 0.5f * (n() - lp); v = lp; }          
        size_t p = 0;
        float prev = 0.f, damp = std::exp(-decay / (float)sr * (float)period);
        for (size_t i = 0; i < len && a + i < bus.size(); ++i) {
            float cur = d[p];
            float out = 0.5f * (cur + prev) * damp;                        
            prev = cur;
            d[p] = out;
            p = (p + 1) % d.size();
            bus[a + i] += vel * cur * std::min(1.f, (float)(len - i) / (0.05f * sr)) * std::min(1.f, (float)i / (0.002f * sr));
        }
    }
    void melody(const Note* m, int count, int bar, int transpose, float vel, int kind, int skipFrom = 999) {
        for (int i = 0; i < count; ++i) {
            if (m[i].step >= skipFrom) continue;
            double t0 = T(bar, m[i].step), dur = m[i].len * step * 0.92;
            float midi = (float)(m[i].midi + transpose);
            if (kind == 0) synthLead(t0, midi, dur, vel);
            else if (kind == 1) { kalimba(t0, midi, vel); flute(t0, midi + 12.f, dur, vel * 0.18f); }
            else flute(t0, midi, dur, vel);                                    
        }
    }
};


void reverb(const std::vector<float>& in, std::vector<float>& out, int sr, int spread) {
    const int combD[4] = {1557, 1617, 1491, 1422}, apD[2] = {225, 556};
    float scale = sr / 44100.f;
    std::vector<float> acc(in.size(), 0.f);
    for (int c = 0; c < 4; ++c) {
        size_t d = (size_t)((combD[c] + spread) * scale);
        std::vector<float> buf(d, 0.f);
        float lp = 0.f;
        for (size_t i = 0, p = 0; i < in.size(); ++i, p = (p + 1) % d) {
            float y = buf[p];
            lp = y * 0.6f + lp * 0.4f;
            buf[p] = in[i] + lp * 0.8f;
            acc[i] += y;
        }
    }
    for (int k = 0; k < 2; ++k) {
        size_t d = (size_t)((apD[k] + spread) * scale);
        std::vector<float> buf(d, 0.f);
        for (size_t i = 0, p = 0; i < acc.size(); ++i, p = (p + 1) % d) {
            float b = buf[p], x = acc[i];
            buf[p] = x + b * 0.5f;
            acc[i] = b - x * 0.5f;
        }
    }
    out.swap(acc);
}

struct MixSettings { float duck, echoWet, reverbWet, arpPan; int echoSteps; };

std::vector<float> mixdown(Studio& s, size_t loopN, const MixSettings& ms) {
    const size_t total = s.drums.size();
    const int sr = s.sr;
   
    std::sort(s.kicks.begin(), s.kicks.end());
    size_t k = 0;
    for (size_t i = 0; i < total; ++i) {
        double t = (double)i / sr;
        while (k + 1 < s.kicks.size() && s.kicks[k + 1] <= t) ++k;
        float duck = 1.f;
        if (!s.kicks.empty() && s.kicks[k] <= t) duck = 1.f - ms.duck * (float)std::exp(-(t - s.kicks[k]) * 9.0);
        s.bass[i] *= duck;
        s.chords[i] *= 0.65f + 0.35f * duck;
    }
    // ping-pong echo on the lead
    const size_t D = (size_t)(ms.echoSteps * s.step * sr);
    std::vector<float> eL(total, 0.f), eR(total, 0.f);
    for (size_t i = D; i < total; ++i) {
        eL[i] = s.lead[i - D] + 0.35f * eR[i - D];
        eR[i] = 0.55f * eL[i - D];
    }
    // reverb send
    std::vector<float> send(total), revL, revR;
    for (size_t i = 0; i < total; ++i)
        send[i] = (s.chords[i] * 0.5f + s.lead[i] * 0.6f + s.pad[i] * 0.4f + s.arp[i] * 0.6f + s.drums[i] * 0.04f) * 0.25f;
    reverb(send, revL, sr, 0);
    reverb(send, revR, sr, 23);

    std::vector<float> out(total * 2);
    const size_t haas = (size_t)(0.011 * sr);
    const float aL = 0.5f - ms.arpPan * 0.5f, aR = 0.5f + ms.arpPan * 0.5f;
    for (size_t i = 0; i < total; ++i) {
        float c = s.chords[i], cd = i >= haas ? s.chords[i - haas] : 0.f;    
        float mono = s.drums[i] * 0.9f + s.bass[i] * 0.55f + s.lead[i] + s.pad[i] * 0.12f;
        out[2 * i] = mono + c * 0.75f + cd * 0.35f + (eL[i] * 0.32f + eR[i] * 0.08f) * ms.echoWet + s.hats[i] * 0.7f
                   + s.arp[i] * aL + revL[i] * ms.reverbWet;
        out[2 * i + 1] = mono + cd * 0.75f + c * 0.35f + (eR[i] * 0.32f + eL[i] * 0.08f) * ms.echoWet + s.hats[i] * 0.9f
                       + s.arp[i] * aR + revR[i] * ms.reverbWet;
    }
    // seamless loop: the last notes' tails continue at the start
    for (size_t i = 0; i < total - loopN; ++i) { out[2 * i] += out[2 * (loopN + i)]; out[2 * i + 1] += out[2 * (loopN + i) + 1]; }
    out.resize(loopN * 2);
    float x1[2] = {0.f, 0.f}, y1[2] = {0.f, 0.f};           
    for (size_t i = 0; i < loopN; ++i)
        for (int c = 0; c < 2; ++c) {
            float x = out[2 * i + c], y = x - x1[c] + 0.9993f * y1[c];
            x1[c] = x; y1[c] = y; out[2 * i + c] = y;
        }
    float pk = 0.f;
    for (float v : out) pk = std::max(pk, std::fabs(v));
    float g = 1.3f / std::max(pk, 1e-6f);
    for (float& v : out) v = std::tanh(v * g) * 0.9f;       
    return out;
}

void prepare(Studio& s, int sr, float bpm, int bars, size_t& loopN) {
    s.sr = sr;
    s.step = 60.0 / bpm / 4.0;
    loopN = (size_t)(bars * 16 * s.step * sr);
    const size_t total = loopN + (size_t)(4.0 * sr);
    for (auto* b : {&s.drums, &s.bass, &s.chords, &s.lead, &s.pad, &s.hats, &s.arp}) b->assign(total, 0.f);
}

const Note kVerse[] = {
    {0, 71, 2}, {2, 74, 2}, {4, 71, 2}, {6, 69, 2}, {8, 67, 4}, {12, 69, 2}, {14, 71, 2},
    {16, 74, 4}, {20, 69, 2}, {22, 66, 2}, {24, 69, 6}, {30, 71, 2},
    {32, 71, 2}, {34, 74, 2}, {36, 76, 2}, {38, 74, 2}, {40, 71, 4}, {44, 69, 2}, {46, 67, 2},
    {48, 69, 4}, {52, 67, 2}, {54, 64, 2}, {56, 67, 8}};
const Note kChorus[] = {
    {0, 79, 3}, {3, 78, 1}, {4, 76, 2}, {6, 74, 2}, {8, 76, 2}, {10, 74, 2}, {12, 71, 4},
    {16, 74, 3}, {19, 76, 1}, {20, 78, 2}, {22, 74, 2}, {24, 69, 8},
    {32, 76, 3}, {35, 74, 1}, {36, 71, 2}, {38, 74, 2}, {40, 76, 2}, {42, 79, 2}, {44, 76, 4},
    {48, 74, 2}, {50, 76, 2}, {52, 74, 2}, {54, 71, 2}, {56, 67, 8}};

std::vector<float> cityLights(int sr) {
    Studio s;
    size_t loopN = 0;
    const int BARS = 32;
    prepare(s, sr, 112.f, BARS, loopN);
    const int roots[4] = {43, 38, 40, 36};                                         
    const int chords[4][4] = {{55, 59, 62, 66}, {54, 57, 62, 64}, {55, 59, 64, 67}, {55, 60, 64, 67}};   
    for (int b = 0; b < BARS; ++b) {
        const char sec = b < 8 ? 'A' : b < 16 ? 'B' : b < 24 ? 'C' : 'D';
        const int c = b % 4, R = roots[c];
        
        for (int st : {0, 4, 8, 12}) s.kick(s.T(b, st), 0.8f, 0.05f);
        for (int st : {2, 6, 10, 14}) s.hat(s.T(b, st), 0.09f);
        if (sec != 'A' || b >= 4) for (int st : {4, 12}) s.clap(s.T(b, st), sec == 'C' ? 0.32f : 0.26f);
        
        for (int st = 0; st < 16; st += 2) s.bassNote(s.T(b, st), (float)(R + (st % 4 ? 12 : 0)), 1.8 * s.step, true);
    
        const int hits[3][2] = {{0, 6}, {6, 4}, {10, 6}};
        for (auto& h : hits)
            for (int k = 0; k < 4; ++k) s.epiano(s.T(b, h[0]), (float)chords[c][k], h[1] * s.step, 0.13f);
        
        s.padChord(s.T(b, 0), chords[c], 3, 16 * s.step, sec == 'A' ? 0.5f : 0.35f);
        
        if (sec == 'C' || (sec == 'D' && b >= 28)) {
            const int pat[8] = {0, 1, 2, 3, 2, 1, 2, 3};
            for (int st = 0; st < 16; st += 2) s.pluck(s.arp, s.T(b, st), (float)(chords[c][pat[st / 2]] + 12), 0.07f, 3.5f, 1.2f, 7.f);
        }
    }
    const int nV = (int)(sizeof kVerse / sizeof kVerse[0]), nC = (int)(sizeof kChorus / sizeof kChorus[0]);
    s.melody(kVerse, nV, 8, 0, 0.30f, 1);
    s.melody(kVerse, nV, 12, 0, 0.30f, 1);
    s.melody(kChorus, nC, 16, 0, 0.32f, 1);
    s.melody(kChorus, nC, 20, 0, 0.32f, 1);
    s.melody(kVerse, nV, 24, 12, 0.24f, 1);
    s.melody(kChorus, nC, 28, 0, 0.32f, 1);
    return mixdown(s, loopN, MixSettings{0.35f, 0.8f, 0.6f, -0.4f, 3});
}

const Note kHook[] = {
    {0, 71, 2}, {2, 76, 2}, {4, 79, 1}, {6, 76, 1}, {7, 79, 1}, {8, 81, 2}, {10, 79, 2}, {12, 76, 2}, {14, 74, 2},
    {16, 76, 3}, {19, 79, 1}, {20, 76, 2}, {22, 74, 2}, {24, 71, 4}, {28, 74, 2}, {30, 76, 2},
    {32, 71, 2}, {34, 74, 2}, {36, 79, 1}, {38, 74, 1}, {39, 79, 1}, {40, 81, 2}, {42, 83, 2}, {44, 81, 2}, {46, 79, 2},
    {48, 78, 3}, {51, 79, 1}, {52, 81, 2}, {54, 78, 2}, {56, 74, 6}};
const Note kHookEnd[] = {{48, 78, 2}, {50, 79, 2}, {52, 81, 2}, {54, 83, 2}, {56, 88, 8}};
const Note kBreak[] = {{0, 76, 8}, {8, 79, 8}, {16, 76, 12}, {28, 74, 4}, {32, 79, 8}, {40, 83, 8}, {48, 81, 16}};
const Note kOutro[] = {{0, 76, 2}, {4, 79, 2}, {8, 83, 4}, {14, 81, 2}, {16, 79, 2}, {20, 76, 2}, {24, 74, 4}, {30, 76, 2},
                       {32, 76, 2}, {36, 79, 2}, {40, 83, 4}, {46, 86, 2}, {48, 83, 4}, {52, 81, 2}, {56, 78, 4}, {60, 74, 4}};

std::vector<float> rooftopRun(int sr) {
    Studio s;
    size_t loopN = 0;
    const int BARS = 32;
    prepare(s, sr, 128.f, BARS, loopN);
    const int roots[4] = {40, 36, 43, 38};
    const int triads[4][3] = {{64, 67, 71}, {60, 64, 67}, {62, 67, 71}, {62, 66, 69}};
    for (int b = 0; b < BARS; ++b) {
        const char sec = b < 8 ? 'A' : b < 16 ? 'B' : b < 20 ? 'C' : b < 28 ? 'D' : 'E';
        const int ch = b % 4, R = roots[ch];
        const bool full = sec != 'C';
        
        if (full) {
            for (int st : {0, 4, 8, 12}) s.kick(s.T(b, st), 1.f);
            for (int st : {3, 6, 11, 14}) s.snare(s.T(b, st), 0.38f);
            if (sec != 'A') for (int st : {4, 12}) s.clap(s.T(b, st), sec == 'D' ? 0.55f : 0.45f);
            for (int st = 0; st < 16; st += 2) s.hat(s.T(b, st), st % 4 ? 0.14f : 0.1f);
        } else {
            for (int st : {0, 8}) s.kick(s.T(b, st), 0.8f);
        }
        
        if (full) {
            const Note bp[7] = {{0, 0, 3}, {3, 0, 1}, {6, 12, 2}, {8, 0, 2}, {11, 7, 1}, {12, 0, 2}, {14, 12, 2}};
            for (const Note& nt : bp) s.bassNote(s.T(b, nt.step), (float)(R + nt.midi), nt.len * s.step * 0.92, false);
        } else {
            s.bassNote(s.T(b, 0), (float)R, 16 * s.step * 0.95, false);
        }
        
        if (full) {
            for (int st : {2, 6, 10, 14}) {
                if (sec == 'A' && b < 4 && (st == 2 || st == 10)) continue;
                for (int k = 0; k < 3; ++k) s.pluck(s.chords, s.T(b, st), (float)triads[ch][k], 0.28f);
            }
        } else {
            s.padChord(s.T(b, 0), triads[ch], 3, 16 * s.step, 1.f);
        }
    }
    const int nHook = (int)(sizeof kHook / sizeof kHook[0]);
    s.melody(kHook, nHook, 8, 0, 0.3f, 0);
    s.melody(kHook, nHook, 12, 0, 0.3f, 0, 48);
    s.melody(kHookEnd, 5, 12, 0, 0.3f, 0);
    s.melody(kBreak, (int)(sizeof kBreak / sizeof kBreak[0]), 16, 0, 0.22f, 0);
    s.melody(kHook, nHook, 20, 12, 0.26f, 0);
    s.melody(kHook, nHook, 24, 12, 0.26f, 0, 48);
    s.melody(kHook, nHook, 20, 0, 0.12f, 0);
    s.melody(kHook, nHook, 24, 0, 0.12f, 0, 48);
    s.melody(kHookEnd, 5, 24, 0, 0.3f, 0);
    s.melody(kOutro, (int)(sizeof kOutro / sizeof kOutro[0]), 28, 0, 0.26f, 0);
    return mixdown(s, loopN, MixSettings{0.55f, 1.f, 0.55f, 0.f, 3});
}

const Note kNeon[] = {
    {0, 76, 4}, {4, 74, 2}, {6, 72, 2}, {8, 74, 4}, {12, 76, 4},
    {16, 77, 4}, {20, 76, 2}, {22, 74, 2}, {24, 72, 8},
    {32, 72, 4}, {36, 74, 2}, {38, 76, 2}, {40, 79, 4}, {44, 76, 4},
    {48, 74, 6}, {54, 72, 2}, {56, 71, 4}, {60, 74, 4}};
const Note kNeonEnd[] = {{32, 76, 4}, {36, 79, 2}, {38, 81, 2}, {40, 84, 4}, {44, 81, 4}, {48, 79, 6}, {54, 76, 2}, {56, 74, 8}};

std::vector<float> neonDrive(int sr) {
    Studio s;
    size_t loopN = 0;
    const int BARS = 32;
    prepare(s, sr, 100.f, BARS, loopN);
    const int roots[4] = {45, 41, 48, 43};                                         // A2 F2 C3 G2
    const int chords[4][3] = {{57, 60, 64}, {57, 60, 65}, {55, 60, 64}, {55, 59, 62}};
    for (int b = 0; b < BARS; ++b) {
        const char sec = b < 8 ? 'A' : b < 16 ? 'B' : b < 24 ? 'C' : 'D';
        const int c = b % 4, R = roots[c];
        if (sec != 'A' || b >= 4) for (int st : {0, 4, 8, 12}) s.kick(s.T(b, st), 0.75f, 0.05f);
        if (sec != 'A') for (int st : {4, 12}) s.snare(s.T(b, st), 0.22f);
        if (sec == 'C' || sec == 'D') for (int st : {2, 6, 10, 14}) s.hat(s.T(b, st), 0.07f);
        for (int st = 0; st < 16; st += 2) s.bassNote(s.T(b, st), (float)(R + (st % 4 ? 12 : 0)), 1.7 * s.step, false);
        s.padChord(s.T(b, 0), chords[c], 3, 16 * s.step, 0.9f);
        const int pat[4] = {0, 1, 2, 1};
        for (int st = 0; st < 16; ++st) s.pluck(s.arp, s.T(b, st), (float)(chords[c][pat[st % 4]] + 12), 0.06f, 1.f, 1.6f, 9.f);
    }
    const int nN = (int)(sizeof kNeon / sizeof kNeon[0]);
    s.melody(kNeon, nN, 16, 0, 0.24f, 0);
    s.melody(kNeon, nN, 20, 0, 0.24f, 0, 32);
    s.melody(kNeonEnd, 8, 20, 0, 0.24f, 0);
    s.melody(kNeon, nN, 24, 12, 0.18f, 0);
    s.melody(kNeon, nN, 28, 0, 0.24f, 0, 32);
    s.melody(kNeonEnd, 8, 28, 0, 0.24f, 0);
    return mixdown(s, loopN, MixSettings{0.3f, 1.f, 0.7f, 0.35f, 3});
}


const Note kSunny[] = {
    {0, 76, 2}, {2, 79, 2}, {4, 84, 4}, {8, 81, 2}, {10, 79, 2}, {12, 76, 4},
    {16, 76, 2}, {18, 74, 2}, {20, 72, 4}, {24, 69, 4}, {28, 72, 4},
    {32, 77, 2}, {34, 76, 2}, {36, 74, 2}, {38, 72, 2}, {40, 74, 4}, {44, 81, 4},
    {48, 79, 6}, {54, 77, 2}, {56, 74, 4}, {60, 71, 4}};
const Note kSunnyEnd[] = {{48, 79, 4}, {52, 77, 2}, {54, 76, 2}, {56, 72, 8}};

std::vector<float> sunnyStreets(int sr) {
    Studio s;
    size_t loopN = 0;
    const int BARS = 32;
    prepare(s, sr, 118.f, BARS, loopN);
    const int roots[4] = {48, 45, 41, 43};                                         // C3 A2 F2 G2
    const int chords[4][4] = {{60, 64, 67, 72}, {57, 60, 64, 69}, {57, 60, 65, 69}, {59, 62, 67, 71}};
    for (int b = 0; b < BARS; ++b) {
        const char sec = b < 8 ? 'A' : b < 16 ? 'B' : b < 24 ? 'C' : 'D';
        const int c = b % 4, R = roots[c];
        for (int st : {0, 10}) s.kick(s.T(b, st), 0.7f, 0.05f);
        if (sec != 'A' || b >= 4) for (int st : {4, 12}) s.clap(s.T(b, st), 0.24f);
        for (int st : {2, 6, 10, 14}) s.hat(s.T(b, st), 0.06f);
        // bass: root and fifth, bouncy
        for (auto nt : {std::make_pair(0, 0), std::make_pair(6, 7), std::make_pair(8, 0), std::make_pair(14, 7)})
            s.bassNote(s.T(b, nt.first), (float)(R + nt.second - 12), 1.6 * s.step, true);
        // ukulele: down-down-up-up-down-up strum, strings a few ms apart
        const int strum[6] = {0, 4, 6, 10, 12, 14};
        for (int k = 0; k < 6; ++k) {
            bool up = strum[k] == 6 || strum[k] == 10 || strum[k] == 14;
            for (int n = 0; n < 4; ++n) {
                int idx = up ? 3 - n : n;
                s.string(s.chords, s.T(b, strum[k]) + n * 0.012, (float)chords[c][idx], up ? 0.16f : 0.22f, 1.2f);
            }
        }
        if (sec == 'C' || sec == 'D')                                            
            for (int st : {0, 3, 6, 8, 11, 14}) s.pluck(s.arp, s.T(b, st), (float)(chords[c][(st / 3) % 4] + 24), 0.05f, 3.5f, 1.2f, 5.f);
    }
    const int nS = (int)(sizeof kSunny / sizeof kSunny[0]);
    s.melody(kSunny, nS, 8, 0, 0.30f, 2);
    s.melody(kSunny, nS, 12, 0, 0.30f, 2, 48);
    s.melody(kSunnyEnd, 4, 12, 0, 0.30f, 2);
    s.melody(kSunny, nS, 16, 0, 0.30f, 1);
    s.melody(kSunny, nS, 20, 0, 0.30f, 1, 48);
    s.melody(kSunnyEnd, 4, 20, 0, 0.30f, 1);
    s.melody(kSunny, nS, 24, 0, 0.26f, 2);
    s.melody(kSunny, nS, 28, 0, 0.26f, 2, 48);
    s.melody(kSunnyEnd, 4, 28, 0, 0.26f, 2);
    return mixdown(s, loopN, MixSettings{0.3f, 0.6f, 0.55f, -0.3f, 3});
}
}  

const char* musicTitle(int track) {
    static const char* const names[MUSIC_TRACKS] = {"CITY LIGHTS", "ROOFTOP RUN", "NEON DRIVE", "SUNNY STREETS"};
    return track >= 0 && track < MUSIC_TRACKS ? names[track] : "OFF";
}

std::vector<float> composeMusic(int sr, int track) {
    switch (track) {
    case 1: return rooftopRun(sr);
    case 2: return neonDrive(sr);
    case 3: return sunnyStreets(sr);
    default: return cityLights(sr);
    }
}
