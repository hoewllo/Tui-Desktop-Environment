#include "workspace.h"
#include "../utils/logger.h"

namespace tdesktop {

WorkspaceManager::WorkspaceManager(WindowManager& wm) : wm_(wm) {
}

void WorkspaceManager::setCount(int count) {
    count_ = std::max(1, std::min(9, count));
}

void WorkspaceManager::setCurrent(int ws) {
    if (ws >= 0 && ws < count_) {
        current_ = ws;
        wm_.setCurrentWorkspace(ws);
        LOG_INFO("Switched to workspace " + std::to_string(ws + 1));
    }
}

void WorkspaceManager::next() {
    setCurrent((current_ + 1) % count_);
}

void WorkspaceManager::prev() {
    setCurrent((current_ - 1 + count_) % count_);
}

void WorkspaceManager::moveWindowToCurrent(int window_id) {
    wm_.moveWindowToWorkspace(window_id, current_);
}

void WorkspaceManager::moveWindowToWorkspace(int window_id, int ws) {
    wm_.moveWindowToWorkspace(window_id, ws);
}

} // namespace tdesktop
