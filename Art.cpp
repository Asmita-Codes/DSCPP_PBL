#include "Art.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace {


inline std::uint32_t hsh(int x, int y, std::uint32_t s) {
    std::uint32_t h = (std::uint32_t)x * 0x8da6b343u ^ (std::uint32_t)y * 0xd8163841u ^ (s + 1u) * 0xcb1ab31fu;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}
inline float h01(int x, int y, std::uint32_t s) { return (float)(hsh(x, y, s) & 0xffffff) / 16777216.f; }
inline float smooth(float t) { return t * t * (3.f - 2.f * t); }
inline float smoothstep(float a, float b, float x) { return smooth(clamp01((x - a) / (b - a))); }

float vnoise(float x, float y, std::uint32_t s, int period = 0) {
    int xi = (int)std::floor(x), yi = (int)std::floor(y);
    float fx = smooth(x - xi), fy = smooth(y - yi);
    auto H = [&](int a, int b) {
        if (period) { a = ((a % period) + period) % period; b = ((b % period) + period) % period; }
        return h01(a, b, s);
    };
    float a = H(xi, yi), b = H(xi + 1, yi), c = H(xi, yi + 1), d = H(xi + 1, yi + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
float fbm(float x, float y, std::uint32_t s, int oct, int period = 0) {
    float sum = 0.f, amp = 0.5f, norm = 0.f;
    for (int i = 0; i < oct; ++i) {
        sum += amp * vnoise(x, y, s + i * 17u, period);
        norm += amp; amp *= 0.5f; x *= 2.f; y *= 2.f;
        if (period) period *= 2;
    }
    return sum / norm;
}


struct Rng {
    std::uint32_t s;
    explicit Rng(std::uint32_t seed) : s(seed * 2654435761u + 12345u) {}
    float f() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float)(s & 0xffffff) / 16777216.f; }
    float r(float a, float b) { return a + (b - a) * f(); }
    int i(int a, int b) { return a + (int)(f() * (float)(b - a + 1)) % (b - a + 1); }
};


struct Canvas {
    int w, h;
    std::vector<std::uint8_t> d;
    Canvas(int W, int H) : w(W), h(H), d((size_t)W * H * 4, 0) {}

    void blend(int x, int y, Col c, float cov) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        float a = c.a / 255.f * cov;
        if (a <= 0.001f) return;
        std::uint8_t* p = &d[((size_t)y * w + x) * 4];
        float da = p[3] / 255.f, oa = a + da * (1.f - a);
        const float src[3] = {(float)c.r, (float)c.g, (float)c.b};
        for (int k = 0; k < 3; ++k) p[k] = (std::uint8_t)std::lround((src[k] * a + p[k] * da * (1.f - a)) / oa);
        p[3] = (std::uint8_t)std::lround(oa * 255.f);
    }
    
    template <class S, class F>
    void shapeF(float x0, float y0, float x1, float y1, S sdf, F colf, float soft = 1.f) {
        int ix0 = std::max(0, (int)std::floor(x0 - soft)), iy0 = std::max(0, (int)std::floor(y0 - soft));
        int ix1 = std::min(w - 1, (int)std::ceil(x1 + soft)), iy1 = std::min(h - 1, (int)std::ceil(y1 + soft));
        for (int y = iy0; y <= iy1; ++y)
            for (int x = ix0; x <= ix1; ++x) {
                float px = x + 0.5f, py = y + 0.5f;
                float cov = clamp01(0.5f - sdf(px, py) / soft);
                if (cov > 0.f) blend(x, y, colf(px, py), cov);
            }
    }
    template <class S>
    void shape(float x0, float y0, float x1, float y1, S sdf, Col c, float soft = 1.f) {
        shapeF(x0, y0, x1, y1, sdf, [c](float, float) { return c; }, soft);
    }
    void circle(float cx, float cy, float r, Col c, float soft = 1.f) {
        shape(cx - r, cy - r, cx + r, cy + r, [=](float x, float y) { return std::hypot(x - cx, y - cy) - r; }, c, soft);
    }
    
    void ball(float cx, float cy, float r, Col c, float rim = 0.45f, float soft = 1.f) {
        shapeF(cx - r, cy - r, cx + r, cy + r, [=](float x, float y) { return std::hypot(x - cx, y - cy) - r; },
               [=](float x, float y) {
                   float t = std::hypot(x - (cx - r * 0.25f), y - (cy - r * 0.25f)) / (r * 1.25f);
                   return c.scaled(1.15f - rim * t * t);
               }, soft);
    }
    void ellipse(float cx, float cy, float rx, float ry, Col c, float rim = 0.f, float soft = 1.f) {
        float m = std::min(rx, ry);
        shapeF(cx - rx, cy - ry, cx + rx, cy + ry,
               [=](float x, float y) { return (std::hypot((x - cx) / rx, (y - cy) / ry) - 1.f) * m; },
               [=](float x, float y) {
                   float t = std::hypot((x - cx) / rx, (y - cy) / ry);
                   return c.scaled(1.1f - rim * t * t);
               }, soft);
    }
    void rrect(float x, float y, float ww, float hh, float r, Col c, float soft = 1.f) {
        float cx = x + ww / 2, cy = y + hh / 2, hx = ww / 2 - r, hy = hh / 2 - r;
        shape(x, y, x + ww, y + hh, [=](float px, float py) {
            float qx = std::fabs(px - cx) - hx, qy = std::fabs(py - cy) - hy;
            return std::hypot(std::max(qx, 0.f), std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f) - r;
        }, c, soft);
    }
    template <class F>
    void rrectF(float x, float y, float ww, float hh, float r, F colf) {
        float cx = x + ww / 2, cy = y + hh / 2, hx = ww / 2 - r, hy = hh / 2 - r;
        shapeF(x, y, x + ww, y + hh, [=](float px, float py) {
            float qx = std::fabs(px - cx) - hx, qy = std::fabs(py - cy) - hy;
            return std::hypot(std::max(qx, 0.f), std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f) - r;
        }, colf);
    }
    void seg(float x0, float y0, float x1, float y1, float th, Col c, float soft = 1.f) {
        float r = th / 2;
        shape(std::min(x0, x1) - r, std::min(y0, y1) - r, std::max(x0, x1) + r, std::max(y0, y1) + r,
              [=](float px, float py) {
                  float bx = x1 - x0, by = y1 - y0, ax = px - x0, ay = py - y0;
                  float t = clamp01((ax * bx + ay * by) / std::max(1e-6f, bx * bx + by * by));
                  return std::hypot(ax - bx * t, ay - by * t) - r;
              }, c, soft);
    }
    void triangle(FPt a, FPt b, FPt c, Col col) {
        float x0 = std::min({a.x, b.x, c.x}), x1 = std::max({a.x, b.x, c.x});
        float y0 = std::min({a.y, b.y, c.y}), y1 = std::max({a.y, b.y, c.y});
        shape(x0, y0, x1, y1, [=](float px, float py) {
            auto edge = [&](FPt p, FPt q) {
                float ex = q.x - p.x, ey = q.y - p.y, l = std::hypot(ex, ey);
                return ((px - p.x) * ey - (py - p.y) * ex) / l;
            };
            float s = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0 ? 1.f : -1.f;
            return std::max({edge(a, b) * s, edge(b, c) * s, edge(c, a) * s});
        }, col);
    }
    // a rotated rectangle (centre, half sizes, angle)
    void orect(float cx, float cy, float hw, float hh, float ang, Col c) {
        float ca = std::cos(ang), sa = std::sin(ang), ext = std::hypot(hw, hh);
        shapeF(cx - ext, cy - ext, cx + ext, cy + ext, [=](float px, float py) {
            float lx = (px - cx) * ca + (py - cy) * sa, ly = -(px - cx) * sa + (py - cy) * ca;
            float qx = std::fabs(lx) - hw, qy = std::fabs(ly) - hh;
            return std::hypot(std::max(qx, 0.f), std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f);
        }, [=](float px, float py) {   // lit from the top-left
            float ly = -(px - cx) * sa + (py - cy) * ca;
            return c.scaled(1.08f - 0.25f * (ly / hh * 0.5f + 0.5f));
        });
    }
    SDL_Texture* upload(SDL_Renderer* r, std::vector<SDL_Texture*>& owner) const {
        SDL_Texture* t = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
        if (!t) return nullptr;
        SDL_UpdateTexture(t, nullptr, d.data(), w * 4);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        owner.push_back(t);
        return t;
    }
};


SDL_Texture* makeRadial(SDL_Renderer* r, std::vector<SDL_Texture*>& o, int n, float power) {
    Canvas c(n, n);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            float d = std::hypot(x + 0.5f - n / 2.f, y + 0.5f - n / 2.f) / (n / 2.f);
            float a = std::pow(clamp01(1.f - d), power);
            std::uint8_t* p = &c.d[((size_t)y * n + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (std::uint8_t)std::lround(a * 255.f);
        }
    return c.upload(r, o);
}

SDL_Texture* makeCone(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    const int W = 512, H = 480;           
    Canvas c(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float dx = x + 0.5f, dy = y + 0.5f - H / 2.f;
            float ang = std::fabs(std::atan2(dy, dx)), dist = std::hypot(dx, dy) / W;
            float angular = 1.f - smoothstep(0.26f, 0.43f, ang);
            float core = 1.f - smoothstep(0.0f, 0.30f, ang) * 0.3f;     
            float radial = std::pow(clamp01(1.f - dist), 1.4f);
            float a = clamp01(angular * core * radial * 1.15f);
            std::uint8_t* p = &c.d[((size_t)y * W + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (std::uint8_t)std::lround(a * 255.f);
        }
    return c.upload(r, o);
}

SDL_Texture* makeVignette(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    const int W = 320, H = 180;
    Canvas c(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float u = (x + 0.5f) / W * 2.f - 1.f, v = (y + 0.5f) / H * 2.f - 1.f;
            float d = std::sqrt(u * u * 0.85f + v * v);
            float a = smoothstep(0.55f, 1.35f, d);
            std::uint8_t* p = &c.d[((size_t)y * W + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (std::uint8_t)std::lround(a * 255.f);
        }
    return c.upload(r, o);
}

SDL_Texture* makeFog(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    const int N = 256;
    Canvas c(N, N);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float n = fbm(x / 64.f, y / 64.f, 91u, 4, 4);                
            float a = smoothstep(0.42f, 0.78f, n);
            std::uint8_t* p = &c.d[((size_t)y * N + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (std::uint8_t)std::lround(a * 255.f);
        }
    return c.upload(r, o);
}

SDL_Texture* makeSmoke(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    const int N = 96;
    Canvas c(N, N);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float d = std::hypot(x + 0.5f - N / 2.f, y + 0.5f - N / 2.f) / (N / 2.f);
            float n = fbm(x / 14.f, y / 14.f, 7u, 3);
            float a = clamp01((1.f - d) * 1.6f) * (0.55f + 0.6f * n);
            a = clamp01(a) * smoothstep(1.0f, 0.6f, d);
            std::uint8_t* p = &c.d[((size_t)y * N + x) * 4];
            p[0] = p[1] = p[2] = 255;
            p[3] = (std::uint8_t)std::lround(clamp01(a) * 255.f);
        }
    return c.upload(r, o);
}

SDL_Texture* makeBlood(SDL_Renderer* r, std::vector<SDL_Texture*>& o, std::uint32_t seed) {
    const int N = 128;
    Canvas c(N, N);
    Rng g(seed);
    std::vector<std::pair<FPt, float>> blobs;
    for (int i = 0; i < 7; ++i) blobs.push_back({FPt{64 + g.r(-18, 18), 64 + g.r(-18, 18)}, g.r(10, 22)});
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float f = 0.f;                                              
            for (auto& b : blobs) {
                float d2 = (x - b.first.x) * (x - b.first.x) + (y - b.first.y) * (y - b.first.y);
                f += b.second * b.second / std::max(d2, 1.f);
            }
            f += (fbm(x / 9.f, y / 9.f, seed, 2) - 0.5f) * 0.6f;
            float a = smoothstep(0.9f, 1.25f, f);
            if (a <= 0.f) continue;
            float dark = clamp01((f - 1.f) * 0.25f);
            c.blend(x, y, mixCol(Col(150, 14, 16), Col(80, 4, 8), dark), a * 0.92f);
        }
    for (int i = 0; i < 14; ++i) {                                     
        float ang = g.r(0, 6.283f), dist = g.r(30, 58);
        c.circle(64 + std::cos(ang) * dist, 64 + std::sin(ang) * dist, g.r(1.5f, 4.f), Col(130, 10, 12, 220));
    }
    return c.upload(r, o);
}

SDL_Texture* makeScorch(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    const int N = 160;
    Canvas c(N, N);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            float dx = x + 0.5f - N / 2.f, dy = y + 0.5f - N / 2.f;
            float d = std::hypot(dx, dy) / (N / 2.f);
            float ang = std::atan2(dy, dx);
            float rays = 0.85f + 0.25f * vnoise(ang * 3.f + 10.f, 0.f, 3u);
            float n = fbm(x / 10.f, y / 10.f, 5u, 3);
            float a = clamp01((rays - d) * 2.2f) * (0.65f + 0.45f * n);
            if (a > 0.f) c.blend(x, y, Col(14, 12, 10), clamp01(a));
        }
    return c.upload(r, o);
}

SDL_Texture* makeCrack(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    Canvas c(64, 64);
    Rng g(77);
    for (int k = 0; k < 6; ++k) {
        float x = g.r(10, 54), y = g.r(10, 54), a = g.r(0, 6.28f);
        for (int s = 0; s < 6; ++s) {
            float nx = x + std::cos(a) * 6.f, ny = y + std::sin(a) * 6.f;
            c.seg(x, y, nx, ny, 1.6f, Col(20, 12, 6, 230));
            x = nx; y = ny; a += g.r(-0.8f, 0.8f);
        }
    }
    for (int k = 0; k < 8; ++k) c.orect(g.r(6, 58), g.r(6, 58), g.r(2, 4), 1.f, g.r(0, 3.14f), Col(200, 160, 100));  // splinters
    return c.upload(r, o);
}


SDL_Texture* makeRubble(SDL_Renderer* r, std::vector<SDL_Texture*>& o, std::uint32_t seed) {
    const int N = 64;                                       
    Canvas c(N, N);
    Rng g(seed);
    for (int y = 0; y < N; ++y)                             
        for (int x = 0; x < N; ++x) {
            float d = std::hypot(x - 32.f, y - 32.f) / 34.f;
            float n = fbm(x / 8.f, y / 8.f, seed, 3);
            float a = clamp01((1.05f - d) * 2.f) * (0.45f + 0.5f * n);
            c.blend(x, y, Col(96, 86, 72), clamp01(a) * 0.85f);
        }
    for (int i = 0; i < 16; ++i) {                          
        float cx = g.r(8, 56), cy = g.r(8, 56), s = g.r(3.f, 8.f);
        bool brick = g.f() < 0.35f;
        Col col = brick ? Col(130, 66, 48) : Col((int)g.r(110, 150), (int)g.r(104, 140), (int)g.r(96, 128));
        float ang = g.r(0, 3.14f);
        c.orect(cx + 2.f, cy + 2.5f, s, s * 0.7f, ang, Col(20, 18, 16, 120));   
        c.orect(cx, cy, s, s * (brick ? 0.5f : 0.75f), ang, col);
    }
    for (int i = 0; i < 20; ++i) c.circle(g.r(4, 60), g.r(4, 60), g.r(0.8f, 1.8f), Col(70, 64, 56));
    return c.upload(r, o);
}

SDL_Texture* makeBarricade(SDL_Renderer* r, std::vector<SDL_Texture*>& o) {
    const int N = 64;
    Canvas c(N, N);
    c.rrect(4, 6, 58, 56, 6, Col(0, 0, 0, 110), 4.f);                     
    c.rrect(3, 3, 58, 58, 5, Col(54, 44, 34));                              
    auto plank = [&](float y, float ang, float len) {
        float cx = 32, cy = y;
        float ca = std::cos(ang), sa = std::sin(ang);
        c.shapeF(0, 0, 64, 64, [=](float px, float py) {
            float lx = (px - cx) * ca + (py - cy) * sa, ly = -(px - cx) * sa + (py - cy) * ca;
            float qx = std::fabs(lx) - len, qy = std::fabs(ly) - 6.f;
            return std::hypot(std::max(qx, 0.f), std::max(qy, 0.f)) + std::min(std::max(qx, qy), 0.f) - 1.f;
        }, [=](float px, float py) {
            float lx = (px - cx) * ca + (py - cy) * sa, ly = -(px - cx) * sa + (py - cy) * ca;
            float grain = 0.88f + 0.12f * std::sin(ly * 1.7f + std::sin(lx * 0.15f) * 2.f);
            float edge = 1.f - 0.3f * smoothstep(3.5f, 6.f, std::fabs(ly));
            return Col(176, 122, 66).scaled(grain * edge);
        });
        for (int s = -1; s <= 1; s += 2) c.ball(cx + ca * len * 0.8f * s, cy + sa * len * 0.8f * s, 1.8f, Col(190, 190, 190));
    };
    plank(18, 0.08f, 26);
    plank(46, -0.06f, 26);
    plank(32, 0.78f, 32);
    c.rrect(3, 3, 58, 58, 5, Col(0, 0, 0, 0));
    for (float t : {6.f, 58.f}) { c.ball(t, 6.f, 3.f, Col(140, 140, 150)); c.ball(t, 58.f, 3.f, Col(140, 140, 150)); }   // bolts
    return c.upload(r, o);
}


struct ZStyle { Col shirt, skin, hair; float bodyRx, bodyRy, armTh, headR; };

SDL_Texture* makeZombie(SDL_Renderer* r, std::vector<SDL_Texture*>& o, ZombieKind k) {
    const int N = 128;
    const float R = N / 4.4f, cx = 60.f, cy = 64.f;          
    Canvas c(N, N);
    ZStyle s{};
    switch (k) {
    case ZombieKind::Walker:  s = {Col(98, 78, 60), Col(128, 152, 104), Col(52, 44, 36), 0.72f, 1.10f, 0.36f, 0.50f}; break;
    case ZombieKind::Runner:  s = {Col(190, 104, 40), Col(172, 176, 146), Col(70, 40, 30), 0.62f, 0.98f, 0.30f, 0.46f}; break;
    case ZombieKind::Tank:    s = {Col(96, 62, 120), Col(104, 126, 92), Col(30, 30, 30), 0.86f, 1.28f, 0.48f, 0.46f}; break;
    case ZombieKind::Spitter: s = {Col(66, 92, 44), Col(150, 205, 76), Col(60, 90, 30), 0.74f, 1.08f, 0.32f, 0.58f}; break;
    case ZombieKind::Boss:    s = {Col(120, 28, 30), Col(96, 70, 66), Col(30, 10, 10), 0.86f, 1.22f, 0.46f, 0.50f}; break;
    }
    Rng g(31u + (std::uint32_t)k * 7u);
    
    for (int side = -1; side <= 1; side += 2) {
        float sx = cx + R * 0.05f, sy = cy + side * s.bodyRy * R * 0.78f;
        float hx = cx + R * 1.55f, hy = cy + side * R * (0.52f + (side > 0 ? 0.06f : 0.f));
        c.seg(sx, sy, hx, hy, s.armTh * R * 2.f + 2.f, Col(0, 0, 0, 90));
        c.seg(sx, sy, hx, hy, s.armTh * R * 2.f, s.shirt.scaled(0.9f));                       
        c.seg(sx + (hx - sx) * 0.55f, sy + (hy - sy) * 0.55f, hx, hy, s.armTh * R * 1.7f, s.skin.scaled(0.92f));
        c.ball(hx + 2.f, hy, s.armTh * R * 1.05f, s.skin);                                     
    }
    
    c.ellipse(cx, cy, s.bodyRx * R + 2.f, s.bodyRy * R + 2.f, Col(0, 0, 0, 110));
    c.ellipse(cx, cy, s.bodyRx * R, s.bodyRy * R, s.shirt, 0.45f);
    for (int i = 0; i < 6; ++i)                                                                 
        c.circle(cx + g.r(-0.5f, 0.5f) * R, cy + g.r(-0.9f, 0.9f) * R, g.r(1.5f, 4.f),
                 i < 3 ? Col(110, 12, 14, 200) : s.shirt.scaled(0.6f));
    if (k == ZombieKind::Boss) {                                                                
        for (int side = -1; side <= 1; side += 2)
            for (int j = 0; j < 3; ++j) {
                float bx = cx - R * 0.3f + j * R * 0.32f, by = cy + side * s.bodyRy * R * 0.85f;
                c.triangle(FPt{bx - 5, by}, FPt{bx + 5, by}, FPt{bx, by + side * 13.f}, Col(210, 200, 180));
            }
    }
    if (k == ZombieKind::Spitter)                                                              
        for (int i = 0; i < 4; ++i) c.ball(cx + g.r(-0.4f, 0.3f) * R, cy + g.r(-0.7f, 0.7f) * R, g.r(3.f, 5.5f), Col(190, 255, 90));
    
    float hx = cx + R * 0.28f, hr = s.headR * R;
    c.circle(hx + 1.5f, cy + 2.f, hr + 1.5f, Col(0, 0, 0, 120));
    c.ball(hx, cy, hr, s.skin, 0.5f);
    c.ellipse(hx - hr * 0.35f, cy, hr * 0.62f, hr * 0.85f, s.hair, 0.3f);                        
    c.circle(hx + hr * 0.45f, cy - hr * 0.35f, 1.6f, Col(220, 40, 30));                          
    c.circle(hx + hr * 0.45f, cy + hr * 0.35f, 1.6f, Col(220, 40, 30));
    if (k == ZombieKind::Boss)
        for (int side = -1; side <= 1; side += 2)
            c.triangle(FPt{hx - 2, cy + side * hr * 0.5f}, FPt{hx + 6, cy + side * hr * 0.6f}, FPt{hx + 10, cy + side * (hr + 9.f)},
                       Col(225, 215, 190));
    return c.upload(r, o);
}


struct Team { Col vest, sleeve, pack, visor, strap; };
const Team kTeams[2] = {
    {Col(40, 132, 225), Col(30, 104, 190), Col(235, 140, 40), Col(30, 170, 235), Col(20, 70, 140)},     
    {Col(228, 62, 150), Col(186, 42, 120), Col(50, 205, 175), Col(255, 150, 210), Col(130, 20, 80)},   
};

SDL_Texture* makePlayer(SDL_Renderer* r, std::vector<SDL_Texture*>& o, bool rifle, int team) {
    const int N = 128;
    const float R = N / 5.4f, cx = 58.f, cy = 64.f;
    Canvas c(N, N);
    const Team& T = kTeams[team];
    const Col glove(28, 30, 36), gun(30, 32, 38), helmet(236, 240, 245), ink(8, 12, 20, 235);
    c.rrect(cx - R * 1.25f - 2.f, cy - R * 0.55f - 2.f, R * 0.7f + 4.f, R * 1.1f + 4.f, 6, ink);          
    c.rrect(cx - R * 1.25f, cy - R * 0.55f, R * 0.7f, R * 1.1f, 4, T.pack);
    float gunY = cy + R * 0.12f;
    if (rifle) {
        c.rrect(cx + R * 0.4f - 2.f, gunY - 5.f, R * 2.05f + 4.f, 10.f, 3.f, ink);
        c.rrect(cx + R * 0.4f, gunY - 3.f, R * 2.0f, 6.f, 2.f, gun);
        c.rrect(cx + R * 1.1f, gunY + 2.f, 6.f, 7.f, 1.5f, Col(24, 24, 26));
        c.rrect(cx + R * 2.2f, gunY - 1.5f, R * 0.35f, 3.f, 1.f, Col(20, 20, 22));
        c.seg(cx, cy + R * 0.8f, cx + R * 0.75f, gunY + 2.f, R * 0.5f + 4.f, ink);
        c.seg(cx + R * 0.1f, cy - R * 0.8f, cx + R * 1.45f, gunY - 1.f, R * 0.48f + 4.f, ink);
        c.seg(cx, cy + R * 0.8f, cx + R * 0.75f, gunY + 2.f, R * 0.5f, T.sleeve);
        c.ball(cx + R * 0.8f, gunY + 1.f, R * 0.24f, glove);
        c.seg(cx + R * 0.1f, cy - R * 0.8f, cx + R * 1.45f, gunY - 1.f, R * 0.48f, T.sleeve);
        c.ball(cx + R * 1.5f, gunY - 1.f, R * 0.24f, glove);
    } else {
        float tip = cx + R * 1.55f;
        c.seg(cx, cy + R * 0.8f, tip, gunY + 1.f, R * 0.48f + 4.f, ink);
        c.seg(cx, cy - R * 0.8f, tip, gunY - 1.f, R * 0.48f + 4.f, ink);
        c.rrect(tip - 2.f, gunY - 4.5f, R * 0.9f + 4.f, 9.f, 2.5f, ink);
        c.seg(cx, cy + R * 0.8f, tip, gunY + 1.f, R * 0.48f, T.sleeve);
        c.seg(cx, cy - R * 0.8f, tip, gunY - 1.f, R * 0.48f, T.sleeve);
        c.ball(tip, gunY, R * 0.3f, glove);
        c.rrect(tip, gunY - 2.5f, R * 0.9f, 5.f, 1.5f, gun);
    }
    c.ellipse(cx, cy, R * 0.72f + 3.f, R * 1.12f + 3.f, ink);                                              
    c.ellipse(cx, cy, R * 0.72f, R * 1.12f, T.vest, 0.3f);
    for (int i = -1; i <= 1; ++i) c.rrect(cx + R * 0.15f, cy + i * R * 0.42f - 3.f, R * 0.4f, 6.f, 2.f, T.vest.scaled(0.78f));
    c.seg(cx - R * 0.45f, cy - R * 0.95f, cx + R * 0.35f, cy + R * 0.95f, 3.f, T.strap);
    c.circle(cx + R * 0.12f, cy, R * 0.5f + 3.f, ink);                                                    
    c.ball(cx + R * 0.12f, cy, R * 0.5f, helmet, 0.35f);
    c.rrect(cx + R * 0.38f, cy - R * 0.28f, R * 0.18f, R * 0.56f, 2.f, T.visor);
    return c.upload(r, o);
}

SDL_Texture* makePickup(SDL_Renderer* r, std::vector<SDL_Texture*>& o, PickupKind k) {
    const int N = 64;
    Canvas c(N, N);
    switch (k) {
    case PickupKind::Health:
        c.rrect(12, 14, 42, 40, 6, Col(0, 0, 0, 120), 3.f);
        c.rrectF(10, 10, 44, 42, 6, [](float, float y) { return mixCol(Col(250, 250, 250), Col(196, 200, 206), (y - 10) / 42); });
        c.rrect(18, 7, 28, 6, 2, Col(160, 164, 170));                                           
        c.rrect(27, 18, 10, 26, 2, Col(220, 36, 44));
        c.rrect(19, 26, 26, 10, 2, Col(220, 36, 44));
        break;
    case PickupKind::Ammo:
        c.rrect(10, 16, 46, 38, 4, Col(0, 0, 0, 120), 3.f);
        c.rrectF(8, 12, 48, 38, 4, [](float, float y) { return mixCol(Col(112, 128, 72), Col(70, 82, 44), (y - 12) / 38); });
        c.rrect(8, 12, 48, 7, 3, Col(86, 98, 54));
        for (int i = 0; i < 4; ++i) {                                                           
            float x = 15 + i * 10.f;
            c.rrect(x, 20, 7, 20, 3, Col(222, 172, 60));
            c.ball(x + 3.5f, 22, 3.4f, Col(196, 130, 70));
        }
        c.rrect(14, 42, 36, 5, 2, Col(230, 210, 120));                                          
        break;
    case PickupKind::Grenade:
        c.ellipse(34, 38, 15, 18, Col(0, 0, 0, 110), 0.f, 3.f);
        c.ellipse(32, 36, 14, 17, Col(76, 110, 60), 0.45f);
        for (int i = -1; i <= 1; ++i) c.seg(19, 36 + i * 8.f, 45, 36 + i * 8.f, 1.5f, Col(40, 60, 32));
        c.seg(32, 20, 32, 52, 1.5f, Col(40, 60, 32));
        c.rrect(27, 12, 10, 8, 2, Col(170, 172, 178));                                          
        c.seg(37, 15, 48, 30, 3.f, Col(150, 152, 160));                                         
        break;
    case PickupKind::Fuel:
        c.rrect(14, 14, 40, 46, 5, Col(0, 0, 0, 120), 3.f);
        c.rrectF(12, 12, 40, 46, 5, [](float x, float) { return mixCol(Col(232, 50, 44), Col(160, 20, 22), (x - 12) / 40); });
        c.seg(18, 20, 46, 52, 4.f, Col(150, 18, 20));                                           
        c.seg(46, 20, 18, 52, 4.f, Col(150, 18, 20));
        c.rrect(16, 6, 14, 9, 3, Col(200, 30, 30));                                            
        c.rrect(38, 5, 10, 10, 3, Col(250, 210, 60));                                           
        break;
    }
    return c.upload(r, o);
}


SDL_Texture* makeIcon(SDL_Renderer* r, std::vector<SDL_Texture*>& o, int which) {
    Canvas c(96, 64);
    const Col m(225, 230, 236), d(150, 156, 166);
    if (which == 0) {            
        c.rrect(6, 26, 22, 12, 3, d); c.rrect(24, 22, 46, 12, 2, m); c.rrect(68, 25, 24, 5, 2, m);
        c.rrect(40, 32, 9, 16, 2, d); c.rrect(28, 32, 7, 13, 2, m); c.rrect(44, 17, 16, 5, 2, d);
    } else if (which == 1) {     
        c.rrect(22, 22, 48, 13, 3, m); c.rrect(26, 32, 13, 22, 3, d); c.rrect(66, 25, 10, 6, 2, d);
    } else if (which == 2) {     
        c.rrect(14, 25, 40, 14, 4, m); c.rrect(50, 20, 16, 24, 3, d);
        c.triangle(FPt{66, 22}, FPt{92, 8}, FPt{92, 56}, Col(255, 240, 180, 110)); c.triangle(FPt{66, 42}, FPt{92, 56}, FPt{66, 22}, Col(255, 240, 180, 110));
    } else if (which == 3) {     
        c.circle(38, 26, 13, m); c.circle(58, 26, 13, m);
        c.triangle(FPt{26, 33}, FPt{70, 33}, FPt{48, 56}, m);
    } else {                     
        c.triangle(FPt{54, 4}, FPt{30, 36}, FPt{50, 36}, m);
        c.triangle(FPt{46, 28}, FPt{66, 28}, FPt{40, 60}, m);
    }
    return c.upload(r, o);
}

}  


namespace {
struct Building { int x0, y0, x1, y1; int style; std::uint32_t seed; };   

void splitBlock(std::vector<Building>& out, int x0, int y0, int x1, int y1, int depth, Rng& g) {
    int w = x1 - x0, h = y1 - y0;
    bool can = (w >= 6 || h >= 6) && depth < 2 && (depth == 0 || g.f() < 0.65f);
    if (!can) { out.push_back({x0, y0, x1, y1, g.i(0, 4), (std::uint32_t)(g.f() * 1e6f)}); return; }
    if (w >= h) { int s = x0 + std::max(3, std::min(w - 3, (int)std::lround(w * g.r(0.35f, 0.65f)))); splitBlock(out, x0, y0, s, y1, depth + 1, g); splitBlock(out, s, y0, x1, y1, depth + 1, g); }
    else        { int s = y0 + std::max(3, std::min(h - 3, (int)std::lround(h * g.r(0.35f, 0.65f)))); splitBlock(out, x0, y0, x1, s, depth + 1, g); splitBlock(out, x0, s, x1, y1, depth + 1, g); }
}
}  

void Art::bakeGround(Gfx& gfx) {
    const int S = 2;                                       
    const int W = (int)WORLD_W * S, H = (int)WORLD_H * S;
    Graph base(COLS, ROWS);
    base.buildCity();                                      
    auto bld = [&](int x, int y) { return base.isBuilding(Vec2i{x, y}); };
    auto road = [&](int x, int y) { return !bld(x, y) && x >= 0 && y >= 0 && x < COLS && y < ROWS; };

    
    std::vector<int> runH(COLS * ROWS, 0), runV(COLS * ROWS, 0), runV0(COLS * ROWS, 0), runH0(COLS * ROWS, 0);
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            if (!road(x, y)) continue;
            int a = x; while (road(a - 1, y)) --a;
            int b = x; while (road(b + 1, y)) ++b;
            int c = y; while (road(x, c - 1)) --c;
            int d = y; while (road(x, d + 1)) ++d;
            runH[y * COLS + x] = b - a + 1; runH0[y * COLS + x] = a;
            runV[y * COLS + x] = d - c + 1; runV0[y * COLS + x] = c;
        }
    auto isInter = [&](int x, int y) { return road(x, y) && runH[y * COLS + x] > 2 && runV[y * COLS + x] > 2; };

   
    std::vector<Building> houses;
    std::vector<int> owner(COLS * ROWS, -1);
    std::vector<char> seen(COLS * ROWS, 0);
    Rng rng(2024);
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x) {
            if (!bld(x, y) || seen[y * COLS + x]) continue;
            int x1 = x; while (x1 < COLS && bld(x1, y)) ++x1;
            int y1 = y; while (y1 < ROWS && bld(x, y1)) ++y1;
            for (int yy = y; yy < y1; ++yy) for (int xx = x; xx < x1; ++xx) seen[yy * COLS + xx] = 1;
            size_t first = houses.size();
            splitBlock(houses, x, y, x1, y1, 0, rng);
            for (size_t i = first; i < houses.size(); ++i)
                for (int yy = houses[i].y0; yy < houses[i].y1; ++yy)
                    for (int xx = houses[i].x0; xx < houses[i].x1; ++xx) owner[yy * COLS + xx] = (int)i;
        }
    const Col roofCol[5] = {Col(124, 124, 128), Col(64, 66, 74), Col(132, 74, 58), Col(82, 104, 98), Col(150, 138, 112)};

    Canvas cv(W, H);
    auto distToBuildings = [&](float px, float py) {       
        int tx = (int)std::floor(px / TILE), ty = (int)std::floor(py / TILE);
        float best = 99.f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int bx = tx + dx, by = ty + dy;
                if (!bld(bx, by)) continue;
                float x0 = bx * (float)TILE, y0 = by * (float)TILE;
                float ex = std::max({x0 - px, 0.f, px - (x0 + TILE)}), ey = std::max({y0 - py, 0.f, py - (y0 + TILE)});
                best = std::min(best, std::hypot(ex, ey));
            }
        return best;
    };

    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float wx = (x + 0.5f) / S, wy = (y + 0.5f) / S;
            int tx = std::min(COLS - 1, (int)(wx / TILE)), ty = std::min(ROWS - 1, (int)(wy / TILE));
            float grain = h01(x, y, 1u) - 0.5f;
            float r, g, b;
            if (bld(tx, ty)) {
                const Building& hs = houses[(size_t)owner[ty * COLS + tx]];
                float lx = wx - hs.x0 * TILE, ly = wy - hs.y0 * TILE;
                float bw = (hs.x1 - hs.x0) * (float)TILE, bh = (hs.y1 - hs.y0) * (float)TILE;
                Col base = roofCol[hs.style];
                float k = 1.f;
                
                float n = fbm(wx / 40.f, wy / 40.f, hs.seed, 3);
                switch (hs.style) {
                case 0: k *= 0.9f + 0.2f * n + grain * 0.12f; break;                                   
                case 1: k *= 0.92f + 0.14f * n + grain * 0.06f; if (std::fmod(ly + 400.f, 22.f) < 1.2f) k *= 0.78f; break;   
                case 2: k *= 0.86f + 0.14f * std::sin((bw > bh ? ly : lx) * 1.15f) + 0.08f * n; break;  
                case 3: k *= 0.92f + 0.12f * n; if (std::fmod(lx, 32.f) < 1.f || std::fmod(ly, 32.f) < 1.f) k *= 0.8f; break;
                default: k *= 0.9f + 0.18f * n + grain * 0.08f; break;
                }
                
                float e = std::min({lx, ly, bw - lx, bh - ly});
                bool topLeft = std::min(lx, ly) <= std::min(bw - lx, bh - ly);
                if (e < 4.f) k = topLeft ? 1.28f : 0.72f;
                else if (e < 11.f) k *= 1.f - (topLeft ? 0.32f : 0.1f) * (1.f - (e - 4.f) / 7.f);
                
                bool innerEdge = (lx < 1.5f && hs.x0 > 0 && bld(hs.x0 - 1, ty)) || (ly < 1.5f && hs.y0 > 0 && bld(tx, hs.y0 - 1)) ||
                                 (bw - lx < 1.5f && bld(hs.x1, ty)) || (bh - ly < 1.5f && bld(tx, hs.y1));
                if (innerEdge) k = 0.35f;
                r = base.r * k; g = base.g * k; b = base.b * k;
            } else {
                float dB = distToBuildings(wx, wy);
                float stain = fbm(wx / 70.f, wy / 70.f, 3u, 3);
                if (dB < 8.f) {                                    
                    float slab = (std::fmod(wx + 1000.f, 16.f) < 0.8f || std::fmod(wy + 1000.f, 16.f) < 0.8f) ? 0.86f : 1.f;
                    float k = (0.92f + 0.12f * stain + grain * 0.1f) * slab;
                    if (dB > 6.5f) k = 1.25f;                       
                    r = 112 * k; g = 112 * k; b = 108 * k;
                } else {                                           
                    float patch = vnoise(wx / 26.f, wy / 26.f, 9u);
                    float k = 0.8f + 0.32f * stain + grain * 0.16f;
                    if (patch > 0.74f) k *= 0.9f;                  
                    float gutter = 1.f - 0.25f * (1.f - smoothstep(8.f, 12.f, dB));
                    k *= gutter;
                    r = 50 * k; g = 53 * k; b = 60 * k;
                }
                
                float dS = distToBuildings(wx - 9.f, wy - 12.f);
                float sh = 1.f - smoothstep(0.f, 10.f, dS);
                r *= 1.f - 0.5f * sh; g *= 1.f - 0.5f * sh; b *= 1.f - 0.45f * sh;
            }
            std::uint8_t* p = &cv.d[((size_t)y * W + x) * 4];
            p[0] = (std::uint8_t)std::clamp(r, 0.f, 255.f);
            p[1] = (std::uint8_t)std::clamp(g, 0.f, 255.f);
            p[2] = (std::uint8_t)std::clamp(b, 0.f, 255.f);
            p[3] = 255;
        }

    // 
    Rng mr(99);
    auto paint = [&](float x, float y, float w, float h, Col c) {   
        cv.shapeF(x * S, y * S, (x + w) * S, (y + h) * S, [=](float px, float py) {
            float qx = std::fabs(px - (x + w / 2) * S) - w / 2 * S, qy = std::fabs(py - (y + h / 2) * S) - h / 2 * S;
            return std::max(qx, qy);
        }, [=](float px, float py) { float n = fbm(px / 7.f, py / 7.f, 4u, 2); return c.alpha(smoothstep(0.25f, 0.6f, n)); });
    };
    for (int ty = 0; ty < ROWS; ++ty)
        for (int tx = 0; tx < COLS; ++tx) {
            if (!road(tx, ty) || isInter(tx, ty)) continue;
            int i = ty * COLS + tx;
            float X = tx * (float)TILE, Y = ty * (float)TILE;
            if (runV[i] == 2 && runV0[i] == ty) {                 
                paint(X + 4, Y + TILE - 1.5f, 16, 3, Col(232, 196, 80, 210));
                for (int side = -1; side <= 1; side += 2)
                    if (isInter(tx + side, ty)) {
                        float sx = side < 0 ? X + 4 : X + TILE - 20;
                        for (int k = 0; k < 6; ++k) paint(sx, Y + 6 + k * 9.5f, 16, 5, Col(226, 226, 220, 200));
                    }
            }
            if (runH[i] == 2 && runH0[i] == tx) {                 
                paint(X + TILE - 1.5f, Y + 4, 3, 16, Col(232, 196, 80, 210));
                for (int side = -1; side <= 1; side += 2)
                    if (isInter(tx, ty + side)) {
                        float sy = side < 0 ? Y + 4 : Y + TILE - 20;
                        for (int k = 0; k < 6; ++k) paint(X + 6 + k * 9.5f, sy, 5, 16, Col(226, 226, 220, 200));
                    }
            }
            
            float h = h01(tx, ty, 55u);
            float cx = X + 6 + h01(tx, ty, 56u) * 20, cy = Y + 6 + h01(tx, ty, 57u) * 20;
            if (h < 0.05f) {
                cv.circle(cx * S, cy * S, 7.f * S, Col(34, 34, 36));
                cv.circle(cx * S, cy * S, 6.f * S, Col(70, 70, 72));
                for (int k = -2; k <= 2; ++k) cv.seg((cx - 5) * S, (cy + k * 2.2f) * S, (cx + 5) * S, (cy + k * 2.2f) * S, 1.2f, Col(40, 40, 42));
            } else if (h < 0.16f) {
                float a = mr.r(0, 6.28f), px = cx * S, py = cy * S;
                for (int s = 0; s < 7; ++s) {
                    float nx = px + std::cos(a) * 9.f, ny = py + std::sin(a) * 9.f;
                    cv.seg(px, py, nx, ny, 1.4f, Col(24, 25, 28, 200));
                    px = nx; py = ny; a += mr.r(-0.9f, 0.9f);
                }
            } else if (h < 0.24f) {
                cv.circle(cx * S, cy * S, mr.r(6, 12) * S, Col(18, 18, 22, 90), 10.f);
            } else if (h < 0.34f) {
                Col lit = mr.f() < 0.5f ? Col(190, 186, 170, 200) : Col(120, 90, 50, 200);
                cv.orect(cx * S, cy * S, mr.r(2, 4) * S, mr.r(1.5f, 3) * S, mr.r(0, 3.14f), lit);
            }
        }

    
    for (const Building& hs : houses) {
        Rng g(hs.seed);
        float X0 = hs.x0 * (float)TILE + 14, Y0 = hs.y0 * (float)TILE + 14;
        float X1 = hs.x1 * (float)TILE - 14, Y1 = hs.y1 * (float)TILE - 14;
        if (X1 - X0 < 20 || Y1 - Y0 < 20) continue;
        int items = g.i(2, 5);
        for (int k = 0; k < items; ++k) {
            int kind = g.i(0, 4);
            float x = g.r(X0, X1 - 30), y = g.r(Y0, Y1 - 30);
            auto shadow = [&](float w, float h, float rad) { cv.rrect((x + 4) * S, (y + 5) * S, w * S, h * S, rad * S, Col(0, 0, 0, 90), 6.f); };
            if (kind == 0) {                                   
                shadow(26, 18, 2);
                cv.rrectF(x * S, y * S, 26 * S, 18 * S, 2 * S, [&](float, float py) { return mixCol(Col(196, 198, 200), Col(150, 152, 156), (py - y * S) / (18 * S)); });
                cv.circle((x + 9) * S, (y + 9) * S, 6.5f * S, Col(60, 62, 66));
                for (int b = 0; b < 3; ++b) { float a = b * 2.09f; cv.seg((x + 9) * S, (y + 9) * S, (x + 9 + std::cos(a) * 5.5f) * S, (y + 9 + std::sin(a) * 5.5f) * S, 2.4f * S, Col(120, 122, 126)); }
                cv.rrect((x + 18) * S, (y + 3) * S, 5 * S, 12 * S, 1 * S, Col(110, 112, 116));
            } else if (kind == 1) {                            
                shadow(30, 18, 1);
                cv.rrect(x * S, y * S, 30 * S, 18 * S, 1 * S, Col(70, 74, 80));
                cv.rrectF((x + 2) * S, (y + 2) * S, 26 * S, 14 * S, 1 * S, [&](float px, float py) {
                    float t = ((px - x * S) + (py - y * S)) / (44.f * S);
                    return mixCol(Col(120, 170, 200), Col(40, 66, 96), t);
                });
                cv.seg((x + 15) * S, (y + 2) * S, (x + 15) * S, (y + 16) * S, 1.5f * S, Col(70, 74, 80));
            } else if (kind == 2) {                            
                cv.circle((x + 16) * S, (y + 17) * S, 13.f * S, Col(0, 0, 0, 90), 8.f);
                cv.ball((x + 12) * S, (y + 12) * S, 12.f * S, Col(150, 120, 90), 0.5f);
                cv.circle((x + 12) * S, (y + 12) * S, 8.f * S, Col(120, 96, 72));
                cv.circle((x + 12) * S, (y + 12) * S, 3.f * S, Col(90, 72, 56));
            } else if (kind == 3 && hs.style != 2) {           
                for (int i = 0; i < 3; ++i) {
                    float px = x + i * 11.f;
                    cv.rrect((px + 2) * S, (y + 3) * S, 9 * S, 22 * S, 1 * S, Col(0, 0, 0, 80), 4.f);
                    cv.rrectF(px * S, y * S, 9 * S, 22 * S, 1 * S, [&](float qx, float qy) {
                        bool line = std::fmod(qy - y * S, 5.5f * S) < 0.8f * S;
                        return line ? Col(90, 110, 150) : mixCol(Col(40, 60, 110), Col(24, 34, 70), (qx - px * S) / (9.f * S));
                    });
                }
            } else {                                           
                cv.circle((x + 6) * S, (y + 7) * S, 5.f * S, Col(0, 0, 0, 90), 5.f);
                cv.ball((x + 5) * S, (y + 5) * S, 4.5f * S, Col(170, 172, 176), 0.5f);
                cv.circle((x + 5) * S, (y + 5) * S, 2.f * S, Col(40, 40, 44));
                shadow(16, 16, 1);
                cv.rrectF((x + 14) * S, (y + 10) * S, 16 * S, 16 * S, 1 * S, [&](float px, float py) {
                    return mixCol(Col(150, 150, 150), Col(100, 100, 104), ((px - (x + 14) * S) + (py - (y + 10) * S)) / (32.f * S));
                });
            }
        }
    }

    
    lamps.clear();
    for (int ty = 1; ty < ROWS; ++ty)
        for (int tx = 1; tx < COLS; ++tx) {
            if (!isInter(tx, ty) || isInter(tx - 1, ty) || isInter(tx, ty - 1)) continue;   
            const float pts[2][2] = {{tx * (float)TILE - 3.f, ty * (float)TILE - 3.f}, {(tx + 2) * (float)TILE + 3.f, (ty + 2) * (float)TILE + 3.f}};
            const Vec2i corner[2] = {{tx - 1, ty - 1}, {tx + 2, ty + 2}};
            for (int k = 0; k < 2; ++k) {
                if (!bld(corner[k].x, corner[k].y)) continue;
                float lx = pts[k][0], ly = pts[k][1];
                cv.circle((lx + 3) * S, (ly + 4) * S, 5.f * S, Col(0, 0, 0, 110), 6.f);
                cv.ball(lx * S, ly * S, 4.f * S, Col(70, 72, 78), 0.4f);
                float hv = h01(tx, ty * 3 + k, 8u);
                lamps.push_back({lx, ly, hv < 0.62f ? 0 : hv < 0.85f ? 1 : 2, hv * 100.f});
            }
        }

    
    const int CW = 640, CH = 640;                           
    for (int cy = 0; cy < (int)WORLD_H; cy += CH)
        for (int cx = 0; cx < (int)WORLD_W; cx += CW) {
            int w = std::min(CW, (int)WORLD_W - cx), h = std::min(CH, (int)WORLD_H - cy);
            SDL_Texture* t = SDL_CreateTexture(gfx.ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w * S, h * S);
            if (!t) continue;
            SDL_UpdateTexture(t, nullptr, &cv.d[((size_t)(cy * S) * W + cx * S) * 4], W * 4);
            SDL_SetTextureBlendMode(t, SDL_BLENDMODE_NONE);
            all_.push_back(t);
            ground.push_back({t, (float)cx, (float)cy, (float)w, (float)h});
        }
}


bool Art::build(Gfx& g) {
    destroy();
    SDL_Renderer* r = g.ren;
    glow = makeRadial(r, all_, 128, 2.0f);
    soft = makeRadial(r, all_, 64, 1.3f);
    cone = makeCone(r, all_);
    vignette = makeVignette(r, all_);
    fog = makeFog(r, all_);
    smoke = makeSmoke(r, all_);
    for (int i = 0; i < 3; ++i) blood[i] = makeBlood(r, all_, 11u + i * 13u);
    scorch = makeScorch(r, all_);
    crack = makeCrack(r, all_);
    for (int i = 0; i < 3; ++i) rubble[i] = makeRubble(r, all_, 40u + i * 7u);
    barricade = makeBarricade(r, all_);
    for (int team = 0; team < 2; ++team) {
        player[team][0] = makePlayer(r, all_, true, team);
        player[team][1] = makePlayer(r, all_, false, team);
    }
    for (int k = 0; k < 5; ++k) zombie[k] = makeZombie(r, all_, (ZombieKind)k);
    for (int k = 0; k < 4; ++k) pickup[k] = makePickup(r, all_, (PickupKind)k);
    icoRifle = makeIcon(r, all_, 0);
    icoPistol = makeIcon(r, all_, 1);
    icoTorch = makeIcon(r, all_, 2);
    icoHeart = makeIcon(r, all_, 3);
    icoBolt = makeIcon(r, all_, 4);
    bakeGround(g);
    return !ground.empty() && glow && player[0][0];
}

void Art::destroy() {
    for (SDL_Texture* t : all_) SDL_DestroyTexture(t);
    all_.clear();
    ground.clear();
}
