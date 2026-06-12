#include "window.h"
#include "../utils/logger.h"
#include <algorithm>

namespace tdesktop {

WindowManager::WindowManager() = default;

int WindowManager::createWindow(const std::string& title, int w, int h) {
    auto win = std::make_unique<Window>();
    win->id = next_id_++;
    win->title = title;
    win->w = std::max(w, win->min_w);
    win->h = std::max(h, win->min_h);
    win->x = (80 - win->w) / 2;
    win->y = (24 - win->h) / 2;
    win->z_order = max_z_++;
    win->content = std::make_unique<Buffer>(win->w - 2, win->h - 2);
    win->workspace = current_workspace_;

    int id = win->id;
    windows_.push_back(std::move(win));
    focusWindow(id);
    LOG_INFO("Window created: id=" + std::to_string(id) + " '" + title + "'");
    return id;
}

void WindowManager::closeWindow(int id) {
    for (auto it = windows_.begin(); it != windows_.end(); ++it) {
        if ((*it)->id == id) {
            LOG_INFO("Window closed: id=" + std::to_string(id));
            windows_.erase(it);
            if (!windows_.empty()) focusWindow(windows_.back()->id);
            return;
        }
    }
}

void WindowManager::closeFocusedWindow() {
    auto* win = getFocusedWindow();
    if (win && win->closable) closeWindow(win->id);
}

Window* WindowManager::getWindow(int id) {
    for (auto& w : windows_) {
        if (w->id == id) return w.get();
    }
    return nullptr;
}

Window* WindowManager::getFocusedWindow() {
    for (auto& w : windows_) {
        if (w->focused) return w.get();
    }
    if (!windows_.empty()) {
        windows_.back()->focused = true;
        return windows_.back().get();
    }
    return nullptr;
}

Window* WindowManager::getWindowAt(int x, int y) {
    for (int i = static_cast<int>(windows_.size()) - 1; i >= 0; --i) {
        if (windows_[i]->visible && windows_[i]->contains(x, y)) {
            return windows_[i].get();
        }
    }
    return nullptr;
}

void WindowManager::focusWindow(int id) {
    for (auto& w : windows_) {
        w->focused = (w->id == id);
        if (w->id == id) raiseWindow(id);
    }
}

void WindowManager::focusNext() {
    if (windows_.empty()) return;
    size_t idx = 0;
    for (size_t i = 0; i < windows_.size(); ++i) {
        if (windows_[i]->focused) { idx = i; break; }
    }
    size_t next = (idx + 1) % windows_.size();
    focusWindow(windows_[next]->id);
}

void WindowManager::focusPrev() {
    if (windows_.empty()) return;
    size_t idx = 0;
    for (size_t i = 0; i < windows_.size(); ++i) {
        if (windows_[i]->focused) { idx = i; break; }
    }
    size_t prev = (idx == 0) ? windows_.size() - 1 : idx - 1;
    focusWindow(windows_[prev]->id);
}

void WindowManager::moveWindow(int id, int x, int y) {
    auto* win = getWindow(id);
    if (win) { win->x = x; win->y = y; }
}

void WindowManager::resizeWindow(int id, int w, int h) {
    auto* win = getWindow(id);
    if (!win) return;
    h = std::max(h, win->min_h);
    w = std::max(w, win->min_w);
    win->w = w;
    win->h = h;
    win->content = std::make_unique<Buffer>(w - 2, h - 2);
}

void WindowManager::setWindowTitle(int id, const std::string& title) {
    auto* win = getWindow(id);
    if (win) win->title = title;
}

void WindowManager::moveWindowToWorkspace(int id, int ws) {
    auto* win = getWindow(id);
    if (win && ws >= 0 && ws < workspace_count_) {
        win->workspace = ws;
        LOG_INFO("Window id=" + std::to_string(id) + " moved to workspace " + std::to_string(ws));
    }
}

std::vector<Window*> WindowManager::getWindowsOnWorkspace(int ws) const {
    std::vector<Window*> result;
    for (const auto& w : windows_) {
        if (w->workspace == ws && w->visible) result.push_back(w.get());
    }
    return result;
}

void WindowManager::raiseWindow(int id) {
    auto* win = getWindow(id);
    if (!win) return;
    win->z_order = ++max_z_;
    updateZOrder();
}

void WindowManager::setCurrentWorkspace(int ws) {
    if (ws >= 0 && ws < workspace_count_) {
        current_workspace_ = ws;
    }
}

void WindowManager::clear() {
    windows_.clear();
    next_id_ = 1;
    max_z_ = 0;
}

void WindowManager::updateZOrder() {
    std::sort(windows_.begin(), windows_.end(),
        [](const auto& a, const auto& b) { return a->z_order < b->z_order; });
}

} // namespace tdesktop
