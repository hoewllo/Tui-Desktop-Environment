#include "dbus_listener.h"
#include "../utils/logger.h"
#include "../utils/helpers.h"
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace tdesktop {

DbusListener::DbusListener(NotifyManager& mgr) : mgr_(mgr) {
}

DbusListener::~DbusListener() {
    stop();
}

void DbusListener::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&DbusListener::listenLoop, this);
    LOG_INFO("D-Bus listener started");
}

void DbusListener::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
    LOG_INFO("D-Bus listener stopped");
}

void DbusListener::listenLoop() {
    // Simple D-Bus listener using session bus
    const char* dbus_addr = getenv("DBUS_SESSION_BUS_ADDRESS");
    if (!dbus_addr) {
        LOG_WARN("No D-Bus session bus address available");
        return;
    }

    // Parse the address (unix:path=/path or unix:abstract=/path)
    std::string addr(dbus_addr);
    std::string path;
    if (helpers::startsWith(addr, "unix:path=")) {
        path = addr.substr(10);
    } else if (helpers::startsWith(addr, "unix:abstract=")) {
        path = addr.substr(14);
    } else {
        LOG_WARN("Unsupported D-Bus address format");
        return;
    }

    // Connect to the bus
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create D-Bus socket");
        return;
    }

    struct sockaddr_un sun;
    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;

    if (addr.find("abstract") != std::string::npos) {
        sun.sun_path[0] = '\0';
        memcpy(sun.sun_path + 1, path.c_str(), path.size());
    } else {
        memcpy(sun.sun_path, path.c_str(), path.size());
    }

    if (connect(sock, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
        LOG_ERROR("Failed to connect to D-Bus: " + std::string(strerror(errno)));
        close(sock);
        return;
    }

    // Send Hello
    const char* hello =
        "AUTH EXTERNAL\r\n"
        "NEGOTIATE_UNIX_FD\r\n"
        "BEGIN\r\n";
    write(sock, hello, strlen(hello));

    // Match Notify signals
    // For simplicity, we'll just poll and read
    fd_set fds;
    while (running_) {
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(sock + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(sock, &fds)) {
            char buf[4096];
            ssize_t n = read(sock, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                std::string data(buf);
                // Simple parsing for notify-send calls
                // Real implementation would parse D-Bus message format
                if (data.find("Notify") != std::string::npos) {
                    // Extract title and body from the D-Bus message
                    // This is a simplified version
                    size_t title_pos = data.find("string \"");
                    if (title_pos != std::string::npos) {
                        title_pos += 8;
                        size_t title_end = data.find("\"", title_pos);
                        std::string title = data.substr(title_pos, title_end - title_pos);

                        size_t body_pos = data.find("string \"", title_end);
                        if (body_pos != std::string::npos) {
                            body_pos += 8;
                            size_t body_end = data.find("\"", body_pos);
                            std::string body = data.substr(body_pos, body_end - body_pos);

                            processNotifyEvent("tdesktop", title, body, 0);
                        }
                    }
                }
            }
        }
    }

    close(sock);
}

void DbusListener::processNotifyEvent(const std::string& app, const std::string& title,
                                       const std::string& body, int urgency) {
    NotifyUrgency u = NotifyUrgency::Normal;
    if (urgency == 0) u = NotifyUrgency::Low;
    else if (urgency == 2) u = NotifyUrgency::Critical;

    if (notify_cb_) notify_cb_(title, body, u);
    else mgr_.show(title, body, u);
}

} // namespace tdesktop
