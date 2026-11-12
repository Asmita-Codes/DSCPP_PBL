#ifndef GFX_H
#define GFX_H

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <cstdio>
#include <string>
#include "GameTypes.h"


constexpr float UI_W = 1280.f, UI_H = 720.f, VIEW_W = 960.f, VIEW_H = 720.f;

using Col = sf::Color;
using SVec = sf::Vector2f;

inline SVec sv(Vec2 v) { return SVec(v.x, v.y); }
inline std::uint8_t alpha8(float a) { return (std::uint8_t)(a < 0.f ? 0.f : a > 255.f ? 255.f : a); }
inline std::string mmss(int s) {
    char b[16];
    std::snprintf(b, sizeof b, "%02d:%02d", s / 60, s % 60);
    return b;
}


inline sf::FloatRect menuButton(int i) { return sf::FloatRect(SVec(540.f, 250.f + i * 50.f), SVec(200.f, 42.f)); }    // 7 buttons
inline sf::FloatRect sideButton(int i) { return sf::FloatRect(SVec(VIEW_W + 16.f + i * 58.f, 40.f), SVec(52.f, 30.f)); } // 5 buttons
inline sf::FloatRect backButton() { return sf::FloatRect(SVec(540.f, 640.f), SVec(200.f, 40.f)); }

class Gfx {
public:
    Gfx();

    void createWindow(bool fullscreen);          
    void updateViewports();                      
    bool isFullscreen() const { return fullscreen_; }

    void beginUi() { window.setView(uiView); }
    void beginWorld(SVec center) { worldView.setCenter(center); window.setView(worldView); }
    SVec toUi(sf::Vector2i px) const { return window.mapPixelToCoords(px, uiView); }
    SVec mouseUi() const { return toUi(sf::Mouse::getPosition(window)); }
    SVec toWorld(sf::Vector2i px) const { return window.mapPixelToCoords(px, worldView); }

    
    void text(const std::string& s, float x, float y, unsigned size, Col c = Col::White, int align = 0);
    void rect(float x, float y, float w, float h, Col fill, Col line = Col::Transparent, float th = 0.f);
    void circle(SVec center, float radius, Col fill, Col line = Col::Transparent, float th = 0.f);
    void panel(float x, float y, float w, float h, const std::string& title = "");
    void button(const sf::FloatRect& r, const std::string& label, bool primary = false, unsigned fs = 18);

    sf::RenderWindow window;
    sf::Font font;
    bool fontOk = false;
    sf::View uiView, worldView;

private:
    bool fullscreen_ = false;
};

#endif
