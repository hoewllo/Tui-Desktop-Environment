#include "menu.h"

namespace tdesktop {

Menu::Menu() = default;

void Menu::addItem(const std::string& label, std::function<void()> action, const std::string& shortcut) {
    items_.push_back({label, shortcut, true, false, std::move(action)});
}

void Menu::addSeparator() {
    items_.push_back({"", "", true, true, nullptr});
}

void Menu::clear() {
    items_.clear();
    selected_ = 0;
}

void Menu::show(int x, int y) {
    x_ = x;
    y_ = y;
    visible_ = true;
    selected_ = 0;
}

void Menu::hide() {
    visible_ = false;
}

void Menu::draw(Renderer& r) {
    if (!visible_) return;

    int w = getWidth();
    int h = getHeight();

    // Draw menu background
    for (int row = y_; row < y_ + h; ++row) {
        for (int col = x_; col < x_ + w; ++col) {
            if (row == y_ || row == y_ + h - 1) {
                r.setFGIdx(7);
                r.setBGIdx(0);
                r.drawChar(col, row, row == y_ ? (col == x_ ? L'┌' : col == x_ + w - 1 ? L'┐' : L'─')
                                                : (col == x_ ? L'└' : col == x_ + w - 1 ? L'┘' : L'─'));
            } else if (col == x_ || col == x_ + w - 1) {
                r.setFGIdx(7);
                r.setBGIdx(0);
                r.drawChar(col, row, L'│');
            } else {
                r.drawChar(col, row, L' ');
            }
        }
    }

    // Draw items
    int iy = y_ + 1;
    for (size_t i = 0; i < items_.size(); ++i) {
        auto& item = items_[i];
        if (item.separator) {
            r.setFGIdx(8);
            for (int col = x_ + 1; col < x_ + w - 1; ++col) {
                r.drawChar(col, iy, L'─');
            }
            iy++;
            continue;
        }

        if (static_cast<int>(i) == selected_) {
            r.setFGIdx(0);
            r.setBGIdx(7);
        } else if (!item.enabled) {
            r.setFGIdx(8);
            r.setBGIdx(0);
        } else {
            r.setFGIdx(7);
            r.setBGIdx(0);
        }

        r.drawText(x_ + 1, iy, " " + item.label + " ");

        if (!item.shortcut.empty()) {
            int sx = x_ + w - static_cast<int>(item.shortcut.size()) - 2;
            r.drawText(sx, iy, item.shortcut);
        }

        iy++;
    }
}

bool Menu::handleKey(const KeyEvent& ev) {
    if (!visible_) return false;

    switch (ev.key) {
        case Key::Escape:
            hide();
            return true;
        case Key::Up:
            do {
                selected_ = (selected_ - 1 + static_cast<int>(items_.size())) % items_.size();
            } while (items_[selected_].separator || !items_[selected_].enabled);
            return true;
        case Key::Down:
            do {
                selected_ = (selected_ + 1) % items_.size();
            } while (items_[selected_].separator || !items_[selected_].enabled);
            return true;
        case Key::Enter:
            if (!items_[selected_].separator && items_[selected_].enabled && items_[selected_].action) {
                items_[selected_].action();
                hide();
            }
            return true;
    }
    return false;
}

bool Menu::handleClick(int mx, int my) {
    if (!visible_) return false;
    if (mx < x_ || mx >= x_ + getWidth() || my < y_ || my >= y_ + getHeight()) {
        hide();
        return false;
    }

    int idx = my - y_ - 1;
    if (idx >= 0 && idx < static_cast<int>(items_.size())) {
        if (!items_[idx].separator && items_[idx].enabled && items_[idx].action) {
            items_[idx].action();
            hide();
        }
    }
    return true;
}

int Menu::getWidth() const {
    int maxw = 10;
    for (const auto& item : items_) {
        int len = static_cast<int>(item.label.size()) + 3;
        if (!item.shortcut.empty()) len += static_cast<int>(item.shortcut.size()) + 2;
        if (len > maxw) maxw = len;
    }
    return maxw;
}

int Menu::getHeight() const {
    return static_cast<int>(items_.size()) + 2;
}

} // namespace tdesktop
