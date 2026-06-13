#include "regeditor.h"
#include "../utils/logger.h"
#include <sstream>

namespace tdesktop {

RegEditor::RegEditor(Terminal& term, ColorManager& colors)
    : term_(term), colors_(colors) {
}

void RegEditor::show() {
    visible_ = true;
    selected_ = 0;
    focus_values_ = false;
    key_path_.clear();
    key_path_.push_back("");
    refreshKeys();
}

void RegEditor::hide() {
    visible_ = false;
}

std::string RegEditor::currentKey() const {
    std::string k;
    for (size_t i = 0; i < key_path_.size(); ++i) {
        if (i == 0) {
            k = key_path_[i];
        } else {
            k += "\\" + key_path_[i];
        }
    }
    return k;
}

void RegEditor::refreshKeys() {
    auto& reg = Registry::instance();
    std::string k = currentKey();
    subkeys_ = reg.enumKeys(k);
    values_ = reg.enumValues(k);
}

void RegEditor::draw(Renderer& r) {
    if (!visible_) return;

    int cols = term_.getCols();
    int rows = term_.getRows();

    // Background
    r.setBGIdx(0);
    r.setFGIdx(7);

    // Title bar
    r.setBGIdx(4);
    r.setFGIdx(15);
    for (int x = 0; x < cols; ++x) r.drawChar(x, 0, L' ');
    r.drawText(2, 0, " Registry Editor " + currentKey() + "  [ESC] Close  [Tab] Switch pane");

    // Divider
    r.setBGIdx(0);
    r.setFGIdx(8);
    for (int x = 0; x < cols; ++x) r.drawChar(x, 1, L'─');

    int split = cols / 3;
    int pane_h = rows - 2;

    // Left pane (tree)
    drawTree(r, 0, 2, split, pane_h);

    // Vertical divider
    r.setFGIdx(8);
    for (int y = 2; y < rows; ++y) r.drawChar(split, y, L'│');

    // Right pane (values)
    drawValues(r, split + 1, 2, cols - split - 1, pane_h);
}

void RegEditor::drawTree(Renderer& r, int x, int y, int w, int h) {
    r.setBGIdx(0);
    r.setFGIdx(focus_values_ ? 8 : 15);

    for (int i = 0; i < h && i < static_cast<int>(subkeys_.size()); ++i) {
        int row = y + i;
        bool sel = !focus_values_ && i == selected_;

        if (sel) {
            r.setBGIdx(7);
            r.setFGIdx(0);
        } else {
            r.setBGIdx(0);
            r.setFGIdx(7);
        }

        std::string label = " " + subkeys_[i];
        if (static_cast<int>(label.size()) > w - 1)
            label = label.substr(0, w - 4) + "...";

        for (int cx = x; cx < x + w; ++cx)
            r.drawChar(cx, row, L' ');
        r.drawText(x, row, label);

        if (sel) {
            r.setBGIdx(0);
            r.setFGIdx(7);
        }
    }
}

void RegEditor::drawValues(Renderer& r, int x, int y, int w, int h) {
    auto& reg = Registry::instance();
    std::string k = currentKey();

    r.setBGIdx(0);
    r.setFGIdx(focus_values_ ? 15 : 8);

    for (int i = 0; i < h && i < static_cast<int>(values_.size()); ++i) {
        int row = y + i;
        bool sel = focus_values_ && i == selected_;

        if (sel) {
            r.setBGIdx(7);
            r.setFGIdx(0);
        } else {
            r.setBGIdx(0);
            r.setFGIdx(7);
        }

        std::string val;
        if (reg.valueExists(k, values_[i])) {
            auto v = reg.getString(k, values_[i], "");
            if (v.empty()) {
                int d = reg.getDword(k, values_[i], 0);
                val = std::to_string(d);
            } else {
                val = v;
            }
        }

        std::string line = " " + values_[i] + " = " + val;
        if (static_cast<int>(line.size()) > w - 1)
            line = line.substr(0, w - 4) + "...";

        for (int cx = x; cx < x + w; ++cx)
            r.drawChar(cx, row, L' ');
        r.drawText(x, row, line);

        if (sel) {
            r.setBGIdx(0);
            r.setFGIdx(7);
        }
    }
}

bool RegEditor::handleKey(const KeyEvent& ev) {
    if (!visible_) return false;

    switch (ev.key) {
        case Key::Escape:
            hide();
            return true;
        case Key::Tab:
            focus_values_ = !focus_values_;
            selected_ = 0;
            return true;
        case Key::Up:
            if (selected_ > 0) selected_--;
            return true;
        case Key::Down: {
            int max = focus_values_ ? static_cast<int>(values_.size()) : static_cast<int>(subkeys_.size());
            if (selected_ < max - 1) selected_++;
            return true;
        }
        case Key::Enter: {
            if (!focus_values_ && selected_ < static_cast<int>(subkeys_.size())) {
                key_path_.push_back(subkeys_[selected_]);
                selected_ = 0;
                refreshKeys();
            }
            return true;
        }
        case Key::Backspace: {
            if (!focus_values_ && key_path_.size() > 1) {
                key_path_.pop_back();
                selected_ = 0;
                refreshKeys();
            }
            return true;
        }
        default:
            break;
    }
    return false;
}

} // namespace tdesktop
