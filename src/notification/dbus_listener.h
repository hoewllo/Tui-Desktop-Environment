#pragma once
#include "notify.h"
#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace tdesktop {

class DbusListener {
public:
    DbusListener(NotifyManager& mgr);
    ~DbusListener();

    DbusListener(const DbusListener&) = delete;
    DbusListener& operator=(const DbusListener&) = delete;

    void start();
    void stop();
    bool isRunning() const { return running_; }

    void onNotification(std::function<void(const std::string&, const std::string&, NotifyUrgency)> cb) {
        notify_cb_ = std::move(cb);
    }

private:
    NotifyManager& mgr_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::function<void(const std::string&, const std::string&, NotifyUrgency)> notify_cb_;

    void listenLoop();
    void processNotifyEvent(const std::string& app, const std::string& title,
                            const std::string& body, int urgency);
};

} // namespace tdesktop
