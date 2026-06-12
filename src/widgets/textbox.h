#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "../core/event.h"
#include <string>
#include <functional>

namespace tdesktop {

class TextBox {
public:
    TextBox(int x, int y, int w);
    ~TextBox() = default;

    void draw(Renderer& r);
    bool handleKey(const KeyEvent& ev);
    bool handleClick(int mx, int my);

    void setText(const std::string& text) { text_ = text; cursor_ = static_cast<int>(text.size()); }
    std::string getText() const { return text_; }

    void setFocused(bool f) { focused_ = f; }
    bool isFocused() const { return focused_; }

    void setPos(int x, int y) { x_ = x; y_ = y; }
    void setWidth(int w) { w_ = w; }

    void setPlaceholder(const std::string& p) { placeholder_ = p; }
    void setMaskInput(bool m) { mask_ = m; }

    void onChange(std::function<void(const std::string&)> cb) { change_cb_ = std::move(cb); }

    void clear();

private:
    int x_, y_, w_;
    std::string text_;
    std::string placeholder_;
    bool mask_ = false;
    bool focused_ = false;
    int cursor_ = 0;
    int scroll_ = 0;
    std::function<void(const std::string&)> change_cb_;
};

} // namespace tdesktop
