#include "notify.h"
#include "../utils/logger.h"

namespace tdesktop {

NotifyManager::NotifyManager() = default;

void NotifyManager::show(const std::string& title, const std::string& body, NotifyUrgency urgency) {
    Notification n;
    n.app_name = "tdesktop";
    n.title = title;
    n.body = body;
    n.urgency = urgency;
    n.created_at = std::chrono::steady_clock::now();

    switch (urgency) {
        case NotifyUrgency::Low: n.timeout_ms = 2000; break;
        case NotifyUrgency::Normal: n.timeout_ms = 3000; break;
        case NotifyUrgency::Critical: n.timeout_ms = 10000; break;
    }

    queue_.push_back(n);
    LOG_INFO("Notification: '" + title + "' - " + body);
    processQueue();
}

void NotifyManager::draw(Renderer& r, int cols, int rows) {
    removeExpired();
    processQueue();

    for (size_t i = 0; i < active_.size() && i < static_cast<size_t>(MAX_VISIBLE); ++i) {
        auto& n = active_[i];
        int nx = cols - NOTIFY_WIDTH - 2;
        int ny = 2 + static_cast<int>(i) * (NOTIFY_HEIGHT + 1);

        // Background
        switch (n.urgency) {
            case NotifyUrgency::Critical:
                r.setBGIdx(1); r.setFGIdx(15); break;  // Red bg
            case NotifyUrgency::Normal:
                r.setBGIdx(4); r.setFGIdx(15); break;  // Blue bg
            case NotifyUrgency::Low:
                r.setBGIdx(8); r.setFGIdx(7); break;   // Gray bg
        }

        // Draw border
        r.drawRectOutline({nx, ny, NOTIFY_WIDTH, NOTIFY_HEIGHT});

        // Fill interior
        r.setBGIdx(0);
        r.setFGIdx(7);
        for (int y = ny + 1; y < ny + NOTIFY_HEIGHT - 1; ++y) {
            for (int x = nx + 1; x < nx + NOTIFY_WIDTH - 1; ++x) {
                r.drawChar(x, y, L' ');
            }
        }

        // Title
        r.setFGIdx(15);
        std::string display_title = n.title;
        if (static_cast<int>(display_title.size()) > NOTIFY_WIDTH - 5)
            display_title = display_title.substr(0, NOTIFY_WIDTH - 8) + "...";
        r.drawText(nx + 2, ny + 1, display_title);

        // Body
        r.setFGIdx(7);
        std::string display_body = n.body;
        if (static_cast<int>(display_body.size()) > NOTIFY_WIDTH - 5)
            display_body = display_body.substr(0, NOTIFY_WIDTH - 8) + "...";
        r.drawText(nx + 2, ny + 2, display_body);

        // Close hint
        r.setFGIdx(8);
        r.drawText(nx + NOTIFY_WIDTH - 5, ny + NOTIFY_HEIGHT - 2, "[X]");
    }
}

void NotifyManager::update() {
    removeExpired();
    processQueue();
}

void NotifyManager::dismiss(int idx) {
    if (idx >= 0 && idx < static_cast<int>(active_.size())) {
        if (dismissed_cb_) dismissed_cb_(idx);
        active_.erase(active_.begin() + idx);
        processQueue();
    }
}

void NotifyManager::dismissAll() {
    active_.clear();
    queue_.clear();
}

bool NotifyManager::handleClick(int x, int y) {
    for (size_t i = 0; i < active_.size(); ++i) {
        auto& n = active_[i];
        int nx = 0; // Will be set correctly with cols
        int ny = 2 + static_cast<int>(i) * (NOTIFY_HEIGHT + 1);
        if (x >= nx + NOTIFY_WIDTH - 5 && x < nx + NOTIFY_WIDTH - 1 &&
            y == ny + NOTIFY_HEIGHT - 2) {
            dismiss(static_cast<int>(i));
            return true;
        }
    }
    return false;
}

void NotifyManager::processQueue() {
    while (!queue_.empty() && active_.size() < static_cast<size_t>(MAX_VISIBLE)) {
        active_.push_back(queue_.front());
        queue_.pop_front();
    }
}

void NotifyManager::removeExpired() {
    for (auto it = active_.begin(); it != active_.end();) {
        if (it->expired()) {
            it = active_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace tdesktop
