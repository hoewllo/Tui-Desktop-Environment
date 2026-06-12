#include "button.h"

namespace tdesktop {

Button::Button(int x, int y, int w, int h, const std::string& text)
    : x_(x), y_(y), w_(w), h_(h), text_(text) {
}

void Button::draw(Renderer& r) {
    if (!enabled_) {
        r.setFGIdx(8);
        r.setBGIdx(0);
    } else if (pressed_) {
        r.setFGIdx(0);
        r.setBGIdx(7);
    } else if (hovered_) {
        r.setFGIdx(15);
        r.setBGIdx(4);
    } else {
        r.setFGIdx(7);
        r.setBGIdx(0);
    }

    // Fill button area
    for (int row = y_; row < y_ + h_; ++row) {
        for (int col = x_; col < x_ + w_; ++col) {
            r.drawChar(col, row, L' ');
        }
    }

    // Draw text centered
    int tx = x_ + (w_ - static_cast<int>(text_.size())) / 2;
    int ty = y_ + (h_ - 1) / 2;
    if (tx >= x_ && ty >= y_) {
        r.drawText(tx, ty, text_);
    }
}

bool Button::handleClick(int mx, int my) {
    if (!enabled_ || !contains(mx, my)) return false;
    if (click_cb_) click_cb_();
    return true;
}

bool Button::contains(int mx, int my) const {
    return mx >= x_ && mx < x_ + w_ && my >= y_ && my < y_ + h_;
}

} // namespace tdesktop
