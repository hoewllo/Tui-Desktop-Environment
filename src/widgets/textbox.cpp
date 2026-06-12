#include "textbox.h"

namespace tdesktop {

TextBox::TextBox(int x, int y, int w)
    : x_(x), y_(y), w_(w) {
}

void TextBox::draw(Renderer& r) {
    if (focused_) {
        r.setFGIdx(15);
        r.setBGIdx(0);
    } else {
        r.setFGIdx(7);
        r.setBGIdx(0);
    }

    // Draw background
    for (int col = x_; col < x_ + w_; ++col) {
        r.drawChar(col, y_, L' ');
    }

    // Draw text
    std::string display;
    if (text_.empty() && !focused_) {
        r.setFGIdx(8);
        display = placeholder_;
    } else if (mask_) {
        display = std::string(text_.size(), '*');
    } else {
        display = text_;
    }

    // Handle scrolling
    int visible = w_ - 1;
    if (cursor_ < scroll_) scroll_ = cursor_;
    if (cursor_ >= scroll_ + visible) scroll_ = cursor_ - visible + 1;
    if (scroll_ < 0) scroll_ = 0;

    std::string visibleText = display.substr(scroll_, std::min(visible, static_cast<int>(display.size()) - scroll_));
    r.drawText(x_, y_, visibleText);

    // Draw cursor
    if (focused_) {
        int cx = x_ + cursor_ - scroll_;
        if (cx >= x_ && cx < x_ + w_) {
            r.setFGIdx(15);
            r.setBGIdx(15);
            r.drawChar(cx, y_, L' ');
            // Restore styles
            r.setBGIdx(0);
        }
    }
}

bool TextBox::handleKey(const KeyEvent& ev) {
    if (!focused_) return false;

    switch (ev.key) {
        case Key::Left:
            if (cursor_ > 0) cursor_--;
            return true;
        case Key::Right:
            if (cursor_ < static_cast<int>(text_.size())) cursor_++;
            return true;
        case Key::Home:
            cursor_ = 0;
            return true;
        case Key::End:
            cursor_ = static_cast<int>(text_.size());
            return true;
        case Key::Backspace:
            if (cursor_ > 0) {
                text_.erase(cursor_ - 1, 1);
                cursor_--;
                if (change_cb_) change_cb_(text_);
            }
            return true;
        case Key::Delete:
            if (cursor_ < static_cast<int>(text_.size())) {
                text_.erase(cursor_, 1);
                if (change_cb_) change_cb_(text_);
            }
            return true;
        default:
            if (ev.codepoint >= 32 && ev.codepoint < 127) {
                text_.insert(cursor_, 1, static_cast<char>(ev.codepoint));
                cursor_++;
                if (change_cb_) change_cb_(text_);
                return true;
            }
            break;
    }
    return false;
}

bool TextBox::handleClick(int mx, int my) {
    if (mx >= x_ && mx < x_ + w_ && my == y_) {
        focused_ = true;
        cursor_ = mx - x_ + scroll_;
        if (cursor_ > static_cast<int>(text_.size())) cursor_ = static_cast<int>(text_.size());
        return true;
    }
    focused_ = false;
    return false;
}

void TextBox::clear() {
    text_.clear();
    cursor_ = 0;
    scroll_ = 0;
}

} // namespace tdesktop
