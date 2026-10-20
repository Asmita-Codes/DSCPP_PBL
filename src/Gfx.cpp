#include "Gfx.h"

Gfx::Gfx() {
    const char* fonts[] = {"font.ttf", "assets/font.ttf", "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/segoeui.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/System/Library/Fonts/Supplemental/Arial.ttf"};
    for (const char* f : fonts)
        if (font.openFromFile(f)) { fontOk = true; break; }
}

void Gfx::createWindow(bool fs) {
    fullscreen_ = fs;
    const char* title = "Dynamic Navigation and Entity Management System";
    if (fs) window.create(sf::VideoMode::getDesktopMode(), title, sf::State::Fullscreen);
    else    window.create(sf::VideoMode({1280u, 720u}), title, sf::Style::Default, sf::State::Windowed);
    window.setFramerateLimit(60);
    updateViewports();
}


void Gfx::updateViewports() {
    sf::Vector2u ws = window.getSize();
    if (ws.x == 0 || ws.y == 0) return;
    float wa = (float)ws.x / (float)ws.y, ua = UI_W / UI_H;
    float vw = 1.f, vh = 1.f, vx = 0.f, vy = 0.f;
    if (wa > ua) { vw = ua / wa; vx = (1.f - vw) / 2.f; }
    else         { vh = wa / ua; vy = (1.f - vh) / 2.f; }
    uiView = sf::View(sf::FloatRect(SVec(0.f, 0.f), SVec(UI_W, UI_H)));
    uiView.setViewport(sf::FloatRect(SVec(vx, vy), SVec(vw, vh)));
    worldView.setSize(SVec(VIEW_W, VIEW_H));
    worldView.setCenter(SVec(VIEW_W / 2.f, VIEW_H / 2.f));
    worldView.setViewport(sf::FloatRect(SVec(vx, vy), SVec(vw * (VIEW_W / UI_W), vh)));
}

void Gfx::text(const std::string& s, float x, float y, unsigned size, Col c, int align) {
    if (!fontOk) return;
    sf::Text t(font, s, size);         
    t.setFillColor(c);
    float w = t.getLocalBounds().size.x;
    t.setOrigin(SVec(align == 1 ? w / 2.f : (align == 2 ? w : 0.f), 0.f));
    t.setPosition(SVec(x, y));
    window.draw(t);
}

void Gfx::rect(float x, float y, float w, float h, Col fill, Col line, float th) {
    sf::RectangleShape r(SVec(w, h));
    r.setPosition(SVec(x, y));
    r.setFillColor(fill);
    r.setOutlineColor(line);
    r.setOutlineThickness(th);
    window.draw(r);
}

void Gfx::circle(SVec center, float radius, Col fill, Col line, float th) {
    sf::CircleShape c(radius);
    c.setOrigin(SVec(radius, radius));
    c.setPosition(center);
    c.setFillColor(fill);
    c.setOutlineColor(line);
    c.setOutlineThickness(th);
    window.draw(c);
}

void Gfx::panel(float x, float y, float w, float h, const std::string& title) {
    rect(x, y, w, h, Col(14, 18, 20, 230), Col(60, 70, 76), 1.f);
    if (!title.empty()) text(title, x + w / 2.f, y + 4.f, 14, Col(230, 170, 40), 1);
}

void Gfx::button(const sf::FloatRect& r, const std::string& label, bool primary, unsigned fs) {
    bool hover = r.contains(mouseUi());
    Col fill = primary ? (hover ? Col(40, 140, 80) : Col(30, 110, 60)) : (hover ? Col(56, 62, 72) : Col(28, 32, 38));
    rect(r.position.x, r.position.y, r.size.x, r.size.y, fill, Col(110, 120, 130), 1.f);
    text(label, r.position.x + r.size.x / 2.f, r.position.y + (r.size.y - (float)fs) / 2.f - 2.f, fs, Col::White, 1);
}
