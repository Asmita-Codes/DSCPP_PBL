#ifndef GFX_H
#define GFX_H

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <unordered_map>
#include "GameTypes.h"
#include "SdlInc.h"

constexpr float UI_W = 1280.f, UI_H = 720.f;
constexpr float HUD_H = 108.f;
constexpr float VIEW_W = UI_W;                     
constexpr float VIEW_H = UI_H - HUD_H;              

struct Col {
    Uint8 r = 255, g = 255, b = 255, a = 255;
    constexpr Col() = default;
    constexpr Col(int R, int G, int B, int A = 255) : r((Uint8)R), g((Uint8)G), b((Uint8)B), a((Uint8)A) {}
    Col alpha(float k) const { float v = a * k; return Col(r, g, b, v < 0.f ? 0 : v > 255.f ? 255 : (int)v); }
    Col scaled(float k) const {
        auto c = [k](Uint8 v) { float f = v * k; return f < 0.f ? 0 : f > 255.f ? 255 : (int)f; };
        return Col(c(r), c(g), c(b), a);
    }
};
inline Col mixCol(Col a, Col b, float t) {
    t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
    return Col((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t), (int)(a.a + (b.a - a.a) * t));
}
namespace C {
constexpr Col White(255, 255, 255), Black(0, 0, 0), Clear(0, 0, 0, 0);
constexpr Col Accent(255, 176, 46);     
constexpr Col Danger(240, 70, 60);
constexpr Col Good(90, 225, 130);
constexpr Col Info(110, 210, 255);
constexpr Col Text(232, 236, 240), Dim(150, 160, 172), Faint(100, 108, 120);
constexpr Col Panel(14, 18, 24, 205), PanelLine(255, 255, 255, 26);
}

struct FRect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};
struct FPt { float x = 0, y = 0; };

inline std::uint8_t alpha8(float a) { return (std::uint8_t)(a < 0.f ? 0.f : a > 255.f ? 255.f : a); }
inline float clamp01(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }
inline std::string mmss(int s) {
    char b[16];
    std::snprintf(b, sizeof b, "%02d:%02d", s / 60, s % 60);
    return b;
}


inline FRect menuButton(int i) { return FRect{490.f, 262.f + i * 42.f, 300.f, 36.f}; }   
inline FRect coopStartButton() { return FRect{442.f, 640.f, 190.f, 46.f}; }              
inline FRect coopBackButton() { return FRect{648.f, 640.f, 190.f, 46.f}; }
inline FRect backButton() { return FRect{540.f, 646.f, 200.f, 44.f}; }

enum class Align { Left, Center, Right };
enum class Font { Body, Title, Plain };   

class Gfx {
public:
    Gfx() = default;
    ~Gfx();
    Gfx(const Gfx&) = delete;
    Gfx& operator=(const Gfx&) = delete;

    bool init(bool fullscreen);                 
    void setFullscreen(bool fs);                 
    bool isFullscreen() const { return fullscreen_; }

    void beginFrame();                           
    void endFrame();                             
    void requestScreenshot() { shotPending_ = true; }
    std::string lastScreenshot;

   
    void setOrigin(float x, float y) { ox_ = x; oy_ = y; }
    void ui() { ox_ = oy_ = 0.f; }
    FPt mouseUi() const;                         
    
    void setBlend(SDL_BlendMode m) { SDL_SetRenderDrawBlendMode(ren, m); blend_ = m; }
    void rect(float x, float y, float w, float h, Col c);
    void rectGrad(float x, float y, float w, float h, Col top, Col bottom);
    void rectHGrad(float x, float y, float w, float h, Col left, Col right);
    void outline(float x, float y, float w, float h, Col c, float th = 1.f);
    void roundRect(float x, float y, float w, float h, float r, Col top, Col bottom);
    void roundRect(float x, float y, float w, float h, float r, Col c) { roundRect(x, y, w, h, r, c, c); }
    void roundOutline(float x, float y, float w, float h, float r, Col c, float th = 1.f);
    void circle(float cx, float cy, float r, Col c);
    void circleGrad(float cx, float cy, float r, Col center, Col edge);
    void ring(float cx, float cy, float r, float th, Col c);
    void arc(float cx, float cy, float r, float th, float a0, float a1, Col c);
    void line(float x1, float y1, float x2, float y2, float th, Col c);
    void lineGrad(float x1, float y1, float x2, float y2, float th, Col c1, Col c2);
    void tri(FPt a, FPt b, FPt c, Col col);
    void poly(const FPt* p, int n, Col c);       

    
    void sprite(SDL_Texture* t, float cx, float cy, float w, float h, float deg = 0.f, Col mod = C::White,
                SDL_BlendMode bm = SDL_BLENDMODE_BLEND);
    
    void spritePivot(SDL_Texture* t, float px, float py, float w, float h, float pivotX, float pivotY, float deg,
                     Col mod = C::White, SDL_BlendMode bm = SDL_BLENDMODE_BLEND);
    void image(SDL_Texture* t, float x, float y, float w, float h, Col mod = C::White, SDL_BlendMode bm = SDL_BLENDMODE_BLEND,
               const SDL_Rect* src = nullptr);

   
    float text(const std::string& s, float x, float y, float size, Col c = C::Text, Align a = Align::Left, Font f = Font::Body);
    float textShadow(const std::string& s, float x, float y, float size, Col c = C::Text, Align a = Align::Left,
                     Font f = Font::Body, float off = 2.f);
    float textWidth(const std::string& s, float size, Font f = Font::Body);
    float textMid(const std::string& s, float x, float cy, float size, Col c = C::Text, Align a = Align::Left,
                  Font f = Font::Body);           

    
    void panel(float x, float y, float w, float h, float r = 10.f);
    bool button(const FRect& r, const std::string& label, bool primary = false, float fs = 17.f, bool enabled = true);

    SDL_Texture* makeTarget(int w, int h);       
    void dropTextCache() { clearTextCache(); }   

    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    float pxScale = 1.f;                         
    double time = 0.0;                            
    std::uint64_t frame = 0;

private:
    struct TextTex { SDL_Texture* tex; int w, h; std::uint64_t used; };
    TTF_Font* font(Font f, int px);
    TextTex* textTex(const std::string& s, float size, Font f);
    void clearTextCache();
    void geometry(const SDL_Vertex* v, int nv, const int* idx, int ni, SDL_Texture* tex = nullptr);
    SDL_Vertex vtx(float x, float y, Col c) const;
    void doScreenshot();

    bool fullscreen_ = false, shotPending_ = false;
    float ox_ = 0.f, oy_ = 0.f, cacheScale_ = 0.f;
    SDL_BlendMode blend_ = SDL_BLENDMODE_BLEND;
    std::string fontPath_[3];
    std::map<std::pair<int, int>, TTF_Font*> fonts_;
    std::unordered_map<std::string, TextTex> texts_;
};

#endif
