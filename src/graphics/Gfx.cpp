#include "Gfx.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
bool fileExists(const std::string& p) {
    SDL_RWops* f = SDL_RWFromFile(p.c_str(), "rb");
    if (!f) return false;
    SDL_RWclose(f);
    return true;
}

std::string findFile(std::initializer_list<const char*> names) {
    std::string base;
    if (char* b = SDL_GetBasePath()) { base = b; SDL_free(b); }
    for (const char* n : names) {
        bool absolute = n[0] == '/' || (n[0] && n[1] == ':');
        if (!absolute && !base.empty() && fileExists(base + n)) return base + n;
        if (fileExists(n)) return n;
    }
    return "";
}
}  

Gfx::~Gfx() {
    clearTextCache();
    for (auto& kv : fonts_) TTF_CloseFont(kv.second);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
}

bool Gfx::init(bool fs) {
    SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "permonitorv2");     
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");               
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) { std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return false; }
    if (TTF_Init() != 0) { std::fprintf(stderr, "TTF_Init: %s\n", SDL_GetError()); return false; }

    fullscreen_ = fs;
    Uint32 flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | (fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    win = SDL_CreateWindow("Dynamic Navigation and Entity Management System", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, 1280, 720, flags);
    if (!win) { std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return false; }
    SDL_SetWindowMinimumSize(win, 640, 360);

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);   
    if (!ren) { std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); return false; }
    SDL_RenderSetLogicalSize(ren, (int)UI_W, (int)UI_H);           
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    std::string plain = findFile({"assets/font.ttf", "font.ttf", "C:/Windows/Fonts/arial.ttf",
                                  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf"});
    std::string body = findFile({"assets/ui.ttf", "ui.ttf"});
    std::string title = findFile({"assets/title.ttf", "title.ttf"});
    if (body.empty()) body = plain;
    if (title.empty()) title = body;
    if (plain.empty()) plain = body;
    fontPath_[0] = body; fontPath_[1] = title; fontPath_[2] = plain;
    if (body.empty()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Font missing",
                                 "No font found. Keep the 'assets' folder next to game.exe.", win);
        return false;
    }
    beginFrame();
    return true;
}

void Gfx::setFullscreen(bool fs) {
    fullscreen_ = fs;
    SDL_SetWindowFullscreen(win, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if (!fs) { SDL_SetWindowSize(win, 1280, 720); SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED); }
}

void Gfx::beginFrame() {
    static const Uint64 start = SDL_GetPerformanceCounter();
    time = (double)(SDL_GetPerformanceCounter() - start) / (double)SDL_GetPerformanceFrequency();
    int ow = 1280, oh = 720;
    SDL_GetRendererOutputSize(ren, &ow, &oh);
    pxScale = std::clamp(std::min(ow / UI_W, oh / UI_H), 0.25f, 6.f);
    if (std::fabs(pxScale - cacheScale_) > 0.01f) {               // resolution changed: re-render text
        clearTextCache();
        for (auto& kv : fonts_) TTF_CloseFont(kv.second);
        fonts_.clear();
        cacheScale_ = pxScale;
    }
    SDL_SetRenderTarget(ren, nullptr);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    setBlend(SDL_BLENDMODE_BLEND);
    ui();
    ++frame;
}

void Gfx::endFrame() {
    if (shotPending_) { doScreenshot(); shotPending_ = false; }
    SDL_RenderPresent(ren);
    for (auto it = texts_.begin(); it != texts_.end();) {        
        if (frame - it->second.used > 120) { SDL_DestroyTexture(it->second.tex); it = texts_.erase(it); }
        else ++it;
    }
}

void Gfx::doScreenshot() {
    int ow = 0, oh = 0;
    SDL_GetRendererOutputSize(ren, &ow, &oh);
    SDL_RenderSetLogicalSize(ren, 0, 0);                         
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, ow, oh, 32, SDL_PIXELFORMAT_RGB888);
    if (s && SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_RGB888, s->pixels, s->pitch) == 0) {
        for (int i = 1; i < 1000; ++i) {
            char name[64];
            std::snprintf(name, sizeof name, "screenshot_%03d.bmp", i);
            if (fileExists(name)) continue;
            if (SDL_SaveBMP(s, name) == 0) lastScreenshot = name;
            break;
        }
    }
    if (s) SDL_FreeSurface(s);
    SDL_RenderSetLogicalSize(ren, (int)UI_W, (int)UI_H);
}

FPt Gfx::mouseUi() const {
    int mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    float lx = 0.f, ly = 0.f;
    SDL_RenderWindowToLogical(ren, mx, my, &lx, &ly);
    return FPt{lx, ly};
}

SDL_Texture* Gfx::makeTarget(int w, int h) {
    SDL_Texture* t = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (t) SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}


SDL_Vertex Gfx::vtx(float x, float y, Col c) const {
    SDL_Vertex v;
    v.position.x = x - ox_;
    v.position.y = y - oy_;
    v.color.r = c.r; v.color.g = c.g; v.color.b = c.b; v.color.a = c.a;
    v.tex_coord.x = 0.f; v.tex_coord.y = 0.f;
    return v;
}

void Gfx::geometry(const SDL_Vertex* v, int nv, const int* idx, int ni, SDL_Texture* tex) {
    SDL_RenderGeometry(ren, tex, v, nv, idx, ni);
}

void Gfx::rect(float x, float y, float w, float h, Col c) {
    if (c.a == 0) return;
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    SDL_FRect r{x - ox_, y - oy_, w, h};
    SDL_RenderFillRectF(ren, &r);
}

void Gfx::rectGrad(float x, float y, float w, float h, Col top, Col bottom) {
    SDL_Vertex v[4] = {vtx(x, y, top), vtx(x + w, y, top), vtx(x + w, y + h, bottom), vtx(x, y + h, bottom)};
    int idx[6] = {0, 1, 2, 0, 2, 3};
    geometry(v, 4, idx, 6);
}

void Gfx::rectHGrad(float x, float y, float w, float h, Col left, Col right) {
    SDL_Vertex v[4] = {vtx(x, y, left), vtx(x + w, y, right), vtx(x + w, y + h, right), vtx(x, y + h, left)};
    int idx[6] = {0, 1, 2, 0, 2, 3};
    geometry(v, 4, idx, 6);
}

void Gfx::outline(float x, float y, float w, float h, Col c, float th) {
    rect(x, y, w, th, c);
    rect(x, y + h - th, w, th, c);
    rect(x, y + th, th, h - 2 * th, c);
    rect(x + w - th, y + th, th, h - 2 * th, c);
}

namespace {

void roundPath(std::vector<FPt>& out, float x, float y, float w, float h, float r, int seg) {
    r = std::max(0.f, std::min(r, std::min(w, h) / 2.f));
    const float cx[4] = {x + r, x + w - r, x + w - r, x + r}, cy[4] = {y + r, y + r, y + h - r, y + h - r};
    const float start[4] = {PI, 1.5f * PI, 0.f, 0.5f * PI};
    out.clear();
    for (int k = 0; k < 4; ++k)
        for (int i = 0; i <= seg; ++i) {
            float a = start[k] + (PI / 2.f) * i / seg;
            out.push_back(FPt{cx[k] + std::cos(a) * r, cy[k] + std::sin(a) * r});
        }
}
}  

void Gfx::roundRect(float x, float y, float w, float h, float r, Col top, Col bottom) {
    if (w <= 0 || h <= 0) return;
    static std::vector<FPt> p;
    static std::vector<SDL_Vertex> v;
    static std::vector<int> idx;
    roundPath(p, x, y, w, h, r, std::clamp((int)(r * pxScale * 0.35f) + 2, 2, 10));
    v.clear(); idx.clear();
    auto colAt = [&](float py) { return mixCol(top, bottom, (py - y) / h); };
    v.push_back(vtx(x + w / 2, y + h / 2, colAt(y + h / 2)));
    for (const FPt& q : p) v.push_back(vtx(q.x, q.y, colAt(q.y)));
    int n = (int)p.size();
    for (int i = 0; i < n; ++i) { idx.push_back(0); idx.push_back(1 + i); idx.push_back(1 + (i + 1) % n); }
    geometry(v.data(), (int)v.size(), idx.data(), (int)idx.size());
}

void Gfx::roundOutline(float x, float y, float w, float h, float r, Col c, float th) {
    static std::vector<FPt> a, b;
    static std::vector<SDL_Vertex> v;
    static std::vector<int> idx;
    int seg = std::clamp((int)(r * pxScale * 0.35f) + 2, 2, 10);
    roundPath(a, x, y, w, h, r, seg);
    roundPath(b, x + th, y + th, w - 2 * th, h - 2 * th, std::max(0.f, r - th), seg);
    v.clear(); idx.clear();
    int n = (int)a.size();
    for (int i = 0; i < n; ++i) { v.push_back(vtx(a[i].x, a[i].y, c)); v.push_back(vtx(b[i].x, b[i].y, c)); }
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        int a0 = 2 * i, b0 = 2 * i + 1, a1 = 2 * j, b1 = 2 * j + 1;
        idx.insert(idx.end(), {a0, a1, b1, a0, b1, b0});
    }
    geometry(v.data(), (int)v.size(), idx.data(), (int)idx.size());
}

void Gfx::circleGrad(float cx, float cy, float r, Col center, Col edge) {
    if (r <= 0.f) return;
    int n = std::clamp((int)(r * pxScale * 0.6f) + 10, 12, 96);
    static std::vector<SDL_Vertex> v;
    static std::vector<int> idx;
    v.clear(); idx.clear();
    v.push_back(vtx(cx, cy, center));
    for (int i = 0; i < n; ++i) {
        float a = 2.f * PI * i / n;
        v.push_back(vtx(cx + std::cos(a) * r, cy + std::sin(a) * r, edge));
    }
    for (int i = 0; i < n; ++i) { idx.push_back(0); idx.push_back(1 + i); idx.push_back(1 + (i + 1) % n); }
    geometry(v.data(), (int)v.size(), idx.data(), (int)idx.size());
}

void Gfx::circle(float cx, float cy, float r, Col c) { if (c.a) circleGrad(cx, cy, r, c, c); }

void Gfx::arc(float cx, float cy, float r, float th, float a0, float a1, Col c) {
    if (c.a == 0 || r <= 0.f) return;
    int n = std::clamp((int)(r * pxScale * 0.6f * std::fabs(a1 - a0) / (2 * PI)) + 6, 6, 128);
    static std::vector<SDL_Vertex> v;
    static std::vector<int> idx;
    v.clear(); idx.clear();
    float ro = r + th / 2, ri = std::max(0.f, r - th / 2);
    for (int i = 0; i <= n; ++i) {
        float a = a0 + (a1 - a0) * i / n, ca = std::cos(a), sa = std::sin(a);
        v.push_back(vtx(cx + ca * ro, cy + sa * ro, c));
        v.push_back(vtx(cx + ca * ri, cy + sa * ri, c));
    }
    for (int i = 0; i < n; ++i) {
        int k = 2 * i;
        idx.insert(idx.end(), {k, k + 2, k + 3, k, k + 3, k + 1});
    }
    geometry(v.data(), (int)v.size(), idx.data(), (int)idx.size());
}

void Gfx::ring(float cx, float cy, float r, float th, Col c) { arc(cx, cy, r, th, 0.f, 2.f * PI, c); }

void Gfx::lineGrad(float x1, float y1, float x2, float y2, float th, Col c1, Col c2) {
    float dx = x2 - x1, dy = y2 - y1, l = std::sqrt(dx * dx + dy * dy);
    if (l < 0.001f) return;
    float nx = -dy / l * th / 2, ny = dx / l * th / 2;
    SDL_Vertex v[4] = {vtx(x1 + nx, y1 + ny, c1), vtx(x2 + nx, y2 + ny, c2), vtx(x2 - nx, y2 - ny, c2), vtx(x1 - nx, y1 - ny, c1)};
    int idx[6] = {0, 1, 2, 0, 2, 3};
    geometry(v, 4, idx, 6);
}

void Gfx::line(float x1, float y1, float x2, float y2, float th, Col c) { lineGrad(x1, y1, x2, y2, th, c, c); }

void Gfx::tri(FPt a, FPt b, FPt c, Col col) {
    SDL_Vertex v[3] = {vtx(a.x, a.y, col), vtx(b.x, b.y, col), vtx(c.x, c.y, col)};
    geometry(v, 3, nullptr, 0);
}

void Gfx::poly(const FPt* p, int n, Col c) {
    if (n < 3) return;
    static std::vector<SDL_Vertex> v;
    static std::vector<int> idx;
    v.clear(); idx.clear();
    for (int i = 0; i < n; ++i) v.push_back(vtx(p[i].x, p[i].y, c));
    for (int i = 1; i + 1 < n; ++i) idx.insert(idx.end(), {0, i, i + 1});
    geometry(v.data(), n, idx.data(), (int)idx.size());
}


void Gfx::image(SDL_Texture* t, float x, float y, float w, float h, Col mod, SDL_BlendMode bm, const SDL_Rect* src) {
    if (!t || mod.a == 0) return;
    SDL_SetTextureColorMod(t, mod.r, mod.g, mod.b);
    SDL_SetTextureAlphaMod(t, mod.a);
    SDL_SetTextureBlendMode(t, bm);
    SDL_FRect d{x - ox_, y - oy_, w, h};
    SDL_RenderCopyF(ren, t, src, &d);
}

void Gfx::sprite(SDL_Texture* t, float cx, float cy, float w, float h, float deg, Col mod, SDL_BlendMode bm) {
    if (!t || mod.a == 0) return;
    SDL_SetTextureColorMod(t, mod.r, mod.g, mod.b);
    SDL_SetTextureAlphaMod(t, mod.a);
    SDL_SetTextureBlendMode(t, bm);
    SDL_FRect d{cx - w / 2 - ox_, cy - h / 2 - oy_, w, h};
    SDL_RenderCopyExF(ren, t, nullptr, &d, deg, nullptr, SDL_FLIP_NONE);
}

void Gfx::spritePivot(SDL_Texture* t, float px, float py, float w, float h, float pivX, float pivY, float deg, Col mod,
                      SDL_BlendMode bm) {
    if (!t || mod.a == 0) return;
    SDL_SetTextureColorMod(t, mod.r, mod.g, mod.b);
    SDL_SetTextureAlphaMod(t, mod.a);
    SDL_SetTextureBlendMode(t, bm);
    SDL_FRect d{px - pivX * w - ox_, py - pivY * h - oy_, w, h};
    SDL_FPoint c{pivX * w, pivY * h};
    SDL_RenderCopyExF(ren, t, nullptr, &d, deg, &c, SDL_FLIP_NONE);
}


TTF_Font* Gfx::font(Font f, int px) {
    auto key = std::make_pair((int)f, px);
    auto it = fonts_.find(key);
    if (it != fonts_.end()) return it->second;
    TTF_Font* ft = TTF_OpenFont(fontPath_[(int)f].c_str(), px);
    fonts_[key] = ft;
    return ft;
}

Gfx::TextTex* Gfx::textTex(const std::string& in, float size, Font f) {
    if (in.empty()) return nullptr;
    
    std::string s = in;
    if (f == Font::Title && size < 15.f) {
        s.clear();
        for (char ch : in) { s += ch; if (ch == ' ') s += ' '; }
    }
    int px = std::max(6, (int)std::lround(size * pxScale));
    std::string key = std::to_string((int)f) + ':' + std::to_string(px) + ':' + s;
    auto it = texts_.find(key);
    if (it != texts_.end()) { it->second.used = frame; return &it->second; }
    TTF_Font* ft = font(f, px);
    if (!ft) return nullptr;
    SDL_Color white{255, 255, 255, 255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(ft, s.c_str(), white);
    if (!surf) return nullptr;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    TextTex tt{tex, surf->w, surf->h, frame};
    SDL_FreeSurface(surf);
    if (!tex) return nullptr;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return &(texts_[key] = tt);
}

void Gfx::clearTextCache() {
    for (auto& kv : texts_) SDL_DestroyTexture(kv.second.tex);
    texts_.clear();
}

float Gfx::textWidth(const std::string& s, float size, Font f) {
    TextTex* t = textTex(s, size, f);
    return t ? t->w / pxScale : 0.f;
}

float Gfx::text(const std::string& s, float x, float y, float size, Col c, Align a, Font f) {
    TextTex* t = textTex(s, size, f);
    if (!t) return 0.f;
    float w = t->w / pxScale, h = t->h / pxScale;
    if (a == Align::Center) x -= w / 2.f;
    else if (a == Align::Right) x -= w;
    // snap to real pixels so text stays razor sharp
    float sx = std::round((x - ox_) * pxScale) / pxScale, sy = std::round((y - oy_) * pxScale) / pxScale;
    SDL_SetTextureColorMod(t->tex, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(t->tex, c.a);
    SDL_FRect d{sx, sy, w, h};
    SDL_RenderCopyF(ren, t->tex, nullptr, &d);
    return w;
}

float Gfx::textMid(const std::string& s, float x, float cy, float size, Col c, Align a, Font f) {
    TextTex* t = textTex(s, size, f);
    if (!t) return 0.f;
    return text(s, x, cy - t->h / pxScale / 2.f, size, c, a, f);
}

float Gfx::textShadow(const std::string& s, float x, float y, float size, Col c, Align a, Font f, float off) {
    text(s, x + off * 0.6f, y + off, size, Col(0, 0, 0, (int)(c.a * 0.7f)), a, f);
    return text(s, x, y, size, c, a, f);
}


void Gfx::panel(float x, float y, float w, float h, float r) {
    roundRect(x - 2, y + 3, w + 4, h + 4, r + 2, Col(0, 0, 0, 70));                
    roundRect(x, y, w, h, r, Col(26, 32, 42, 222), Col(12, 15, 21, 232));
    roundOutline(x, y, w, h, r, C::PanelLine, 1.f);
    rectHGrad(x + r, y + 1, (w - 2 * r) / 2, 1, Col(255, 255, 255, 0), Col(255, 255, 255, 40));   
    rectHGrad(x + w / 2, y + 1, (w - 2 * r) / 2, 1, Col(255, 255, 255, 40), Col(255, 255, 255, 0));
}

bool Gfx::button(const FRect& r, const std::string& label, bool primary, float fs, bool enabled) {
    FPt m = mouseUi();
    bool hover = enabled && r.contains(m.x, m.y);
    float rad = 8.f;
    roundRect(r.x, r.y + 3, r.w, r.h, rad, Col(0, 0, 0, 80));
    if (primary) {
        roundRect(r.x, r.y, r.w, r.h, rad, hover ? Col(255, 200, 95) : Col(255, 184, 64), hover ? Col(240, 150, 40) : Col(222, 132, 26));
        roundOutline(r.x, r.y, r.w, r.h, rad, Col(255, 230, 170, 120));
    } else {
        roundRect(r.x, r.y, r.w, r.h, rad, hover ? Col(58, 68, 84, 240) : Col(38, 45, 57, 230), hover ? Col(40, 47, 60, 240) : Col(24, 29, 38, 230));
        roundOutline(r.x, r.y, r.w, r.h, rad, hover ? C::Accent.alpha(0.85f) : Col(255, 255, 255, 30));
        if (hover) roundRect(r.x + 6, r.y + 10, 3, r.h - 20, 1.5f, C::Accent);
    }
    Col tc = primary ? Col(36, 22, 6) : (enabled ? (hover ? C::White : C::Text) : C::Faint);
    textMid(label, r.x + r.w / 2.f, r.y + r.h / 2.f, fs, tc, Align::Center, primary ? Font::Title : Font::Body);
    return hover;
}
