#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <chrono>

namespace tdesktop {

enum class NotifyUrgency {
    Low,
    Normal,
    Critical
};

struct Notification {
    std::string app_name;
    std::string title;
    std::string body;
    NotifyUrgency urgency = NotifyUrgency::Normal;
    std::chrono::steady_clock::time_point created_at;
    int timeout_ms = 3000;

    bool expired() const {
        auto elapsed = std::chrono::steady_clock::now() - created_at;
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > timeout_ms;
    }
};

class NotifyManager {
public:
    NotifyManager();
    ~NotifyManager() = default;

    void show(const std::string& title, const std::string& body, NotifyUrgency urgency = NotifyUrgency::Normal);
    void draw(Renderer& r, int cols, int rows);
    void update();
    void dismiss(int idx);
    void dismissAll();

    bool handleClick(int x, int y);

    void onDismissed(std::function<void(int)> cb) { dismissed_cb_ = std::move(cb); }

    static constexpr int MAX_VISIBLE = 3;
    static constexpr int NOTIFY_WIDTH = 40;
    static constexpr int NOTIFY_HEIGHT = 5;

private:
    std::deque<Notification> queue_;
    std::vector<Notification> active_;

    std::function<void(int)> dismissed_cb_;

    void processQueue();
    void removeExpired();
};

} // namespace tdesktop
