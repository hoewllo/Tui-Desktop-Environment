#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include <string>
#include <functional>

namespace tdesktop {

class Button {
public:
    Button(int x, int y, int w, int h, const std::string& text);
    ~Button() = default;

    void draw(Renderer& r);
    bool handleClick(int mx, int my);
    bool contains(int mx, int my) const;

    void setText(const std::string& text) { text_ = text; }
    std::string getText() const { return text_; }

    void setEnabled(bool e) { enabled_ = e; }
    bool isEnabled() const { return enabled_; }

    void onClick(std::function<void()> cb) { click_cb_ = std::move(cb); }

    void setPos(int x, int y) { x_ = x; y_ = y; }
    void setSize(int w, int h) { w_ = w; h_ = h; }

private:
    int x_, y_, w_, h_;
    std::string text_;
    bool enabled_ = true;
    bool hovered_ = false;
    bool pressed_ = false;
    std::function<void()> click_cb_;
};

} // namespace tdesktop
