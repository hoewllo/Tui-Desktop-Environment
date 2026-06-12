#include "listbox.h"
#include <algorithm>

namespace tdesktop {

ListBox::ListBox(int x, int y, int w, int h)
    : x_(x), y_(y), w_(w), h_(h) {
}

void ListBox::draw(Renderer& r) {
    int start = scroll_;
    int end = std::min(start + h_, static_cast<int>(items_.size()));

    for (int i = start; i < end; ++i) {
        int row = y_ + (i - start);
        bool is_selected = (i == selected_);

        if (is_selected && focused_) {
            r.setFGIdx(0);
            r.setBGIdx(7);
        } else if (is_selected) {
            r.setFGIdx(7);
            r.setBGIdx(4);
        } else {
            r.setFGIdx(7);
            r.setBGIdx(0);
        }

        // Clear row
        for (int col = x_; col < x_ + w_; ++col) {
            r.drawChar(col, row, L' ');
        }

        // Draw item text
        std::string item = items_[i];
        if (static_cast<int>(item.size()) > w_) {
            item = item.substr(0, w_ - 3) + "...";
        }
        r.drawText(x_, row, item);
    }
}

bool ListBox::handleKey(const KeyEvent& ev) {
    if (!focused_ || items_.empty()) return false;

    switch (ev.key) {
        case Key::Up:
            if (selected_ > 0) {
                selected_--;
                if (selected_ < scroll_) scroll_ = selected_;
                if (select_cb_) select_cb_(selected_, items_[selected_]);
            }
            return true;
        case Key::Down:
            if (selected_ < static_cast<int>(items_.size()) - 1) {
                selected_++;
                if (selected_ >= scroll_ + h_) scroll_ = selected_ - h_ + 1;
                if (select_cb_) select_cb_(selected_, items_[selected_]);
            }
            return true;
        case Key::Home:
            selected_ = 0;
            scroll_ = 0;
            if (select_cb_) select_cb_(selected_, items_[selected_]);
            return true;
        case Key::End:
            selected_ = static_cast<int>(items_.size()) - 1;
            scroll_ = std::max(0, selected_ - h_ + 1);
            if (select_cb_) select_cb_(selected_, items_[selected_]);
            return true;
        case Key::PageUp:
            selected_ = std::max(0, selected_ - h_);
            scroll_ = std::max(0, scroll_ - h_);
            if (select_cb_) select_cb_(selected_, items_[selected_]);
            return true;
        case Key::PageDown:
            selected_ = std::min(static_cast<int>(items_.size()) - 1, selected_ + h_);
            scroll_ = std::min(static_cast<int>(items_.size()) - h_, scroll_ + h_);
            if (scroll_ < 0) scroll_ = 0;
            if (select_cb_) select_cb_(selected_, items_[selected_]);
            return true;
        case Key::Enter:
            if (select_cb_) select_cb_(selected_, items_[selected_]);
            return true;
    }
    return false;
}

bool ListBox::handleClick(int mx, int my) {
    if (mx < x_ || mx >= x_ + w_ || my < y_ || my >= y_ + h_) return false;

    int idx = scroll_ + (my - y_);
    if (idx >= 0 && idx < static_cast<int>(items_.size())) {
        selected_ = idx;
        focused_ = true;
        if (select_cb_) select_cb_(selected_, items_[selected_]);
        return true;
    }
    return false;
}

void ListBox::addItem(const std::string& item) {
    items_.push_back(item);
}

void ListBox::removeItem(int idx) {
    if (idx >= 0 && idx < static_cast<int>(items_.size())) {
        items_.erase(items_.begin() + idx);
        if (selected_ >= static_cast<int>(items_.size())) selected_ = static_cast<int>(items_.size()) - 1;
    }
}

void ListBox::clearItems() {
    items_.clear();
    selected_ = 0;
    scroll_ = 0;
}

void ListBox::setItems(const std::vector<std::string>& items) {
    items_ = items;
    selected_ = 0;
    scroll_ = 0;
}

void ListBox::setSelected(int idx) {
    if (idx >= 0 && idx < static_cast<int>(items_.size())) {
        selected_ = idx;
        if (selected_ < scroll_) scroll_ = selected_;
        if (selected_ >= scroll_ + h_) scroll_ = selected_ - h_ + 1;
    }
}

std::string ListBox::getSelectedItem() const {
    if (items_.empty() || selected_ < 0 || selected_ >= static_cast<int>(items_.size()))
        return "";
    return items_[selected_];
}

} // namespace tdesktop
