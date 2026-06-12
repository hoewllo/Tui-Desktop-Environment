#include "taskbar.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace tdesktop {

Taskbar::Taskbar(WindowManager& wm) : wm_(wm) {
}

void Taskbar::draw(Renderer& r, int cols) {
    // Taskbar background
    r.setBGIdx(4);    // Blue background
    r.setFGIdx(15);   // White text
    r.setStyle(false, false, false, false, false, false);
    for (int x = 0; x < cols; ++x) {
        r.drawChar(x, 0, L' ');
    }

    // Draw window buttons
    int btn_x = 1;
    auto windows = wm_.getWindowsOnWorkspace(wm_.getCurrentWorkspace());
    for (auto* win : windows) {
        std::string label = " [" + win->title + "] ";
        if (win->focused) {
            r.setFGIdx(11); // Yellow for focused
        }
        r.drawText(btn_x, 0, label);
        if (win->focused) r.setFGIdx(15);
        btn_x += static_cast<int>(label.size());
        if (btn_x + 20 >= cols) break;
    }

    // Draw clock (right-aligned)
    r.setFGIdx(15);
    int clock_x = cols - static_cast<int>(clock_.size()) - 2;
    if (clock_x > btn_x + 2) {
        r.drawText(clock_x, 0, " " + clock_ + " ");
    }
}

bool Taskbar::handleClick(int x, int y) {
    if (y != 0) return false;

    int btn_x = 1;
    auto windows = wm_.getWindowsOnWorkspace(wm_.getCurrentWorkspace());
    for (auto* win : windows) {
        std::string label = " [" + win->title + "] ";
        int btn_w = static_cast<int>(label.size());
        if (x >= btn_x && x < btn_x + btn_w) {
            if (window_click_cb_) window_click_cb_(win->id);
            return true;
        }
        btn_x += btn_w;
        if (btn_x + 20 >= wm_.getWindowsOnWorkspace(wm_.getCurrentWorkspace()).size() * 10) break;
    }
    return false;
}

} // namespace tdesktop
