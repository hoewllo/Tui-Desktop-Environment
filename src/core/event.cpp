#include "event.h"
#include "../utils/logger.h"
#include <unistd.h>
#include <sys/select.h>
#include <cstring>
#include <sstream>

namespace tdesktop {

EventLoop::EventLoop() = default;

EventLoop::~EventLoop() {
    running_ = false;
}

void EventLoop::init(int stdin_fd) {
    stdin_fd_ = stdin_fd;
    running_ = true;
}

bool EventLoop::poll(Event& ev, int timeout_ms) {
    ev = Event{};

    auto now = std::chrono::steady_clock::now();
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->second.next <= now) {
            Event timer_ev;
            timer_ev.type = Event::Timer;
            timer_ev.timer_id = it->first;
            it->second.callback();
            if (it->second.oneshot) {
                it = timers_.erase(it);
                continue;
            } else {
                it->second.next = now + std::chrono::milliseconds(it->second.interval_ms);
            }
        }
        ++it;
    }

    if (readInput(ev)) return true;

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(stdin_fd_, &fds);

    int ret = select(stdin_fd_ + 1, &fds, nullptr, nullptr, &tv);
    if (ret > 0 && FD_ISSET(stdin_fd_, &fds)) {
        if (readInput(ev)) return true;
    }

    return false;
}

int EventLoop::addTimer(int interval_ms, bool oneshot, std::function<void()> callback) {
    int id = nextTimerId_++;
    TimerInfo info;
    info.interval_ms = interval_ms;
    info.oneshot = oneshot;
    info.next = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms);
    info.callback = std::move(callback);
    timers_[id] = info;
    return id;
}

void EventLoop::removeTimer(int id) {
    timers_.erase(id);
}

bool EventLoop::readInput(Event& ev) {
    char buf[256];
    ssize_t n = read(stdin_fd_, buf, sizeof(buf));
    if (n < 0) return false;
    if (n == 0) {
        ev.type = Event::Quit;
        return true;
    }

    for (ssize_t i = 0; i < n; ++i) {
        // Check for mouse events (CSI sequences)
        if (buf[i] == '\033' && i + 2 < n && buf[i + 1] == '[') {
            // Check if it's a mouse sequence (ends with M or m)
            for (ssize_t j = i + 2; j < n; ++j) {
                if (buf[j] == 'M' || buf[j] == 'm') {
                    size_t param_len = static_cast<size_t>(j - i - 2);
                    if (param_len >= 3) {
                        ev.type = Event::Mouse;
                        ev.mouse = parseMouse(buf + i, static_cast<size_t>(n - i));
                        if (keyCallback_ && ev.type == Event::Key) keyCallback_(ev.key);
                        if (mouseCallback_ && ev.type == Event::Mouse) mouseCallback_(ev.mouse);
                        return true;
                    }
                    break;
                }
            }
        }
    }

    ev.type = Event::Key;
    ev.key = parseKey(buf, static_cast<size_t>(n));
    if (keyCallback_) keyCallback_(ev.key);
    return true;
}

KeyEvent EventLoop::parseKey(const char* buf, size_t len) {
    KeyEvent ev{};

    if (len == 0) return ev;

    // Handle escape sequences
    if (buf[0] == '\033') {
        if (len == 1) {
            ev.key = Key::Escape;
            return ev;
        }
        if (len >= 2 && buf[1] == '[') {
            if (len >= 3) {
                switch (buf[2]) {
                    case 'A': ev.key = Key::Up; return ev;
                    case 'B': ev.key = Key::Down; return ev;
                    case 'C': ev.key = Key::Right; return ev;
                    case 'D': ev.key = Key::Left; return ev;
                    case 'H': ev.key = Key::Home; return ev;
                    case 'F': ev.key = Key::End; return ev;
                    case '5': if (len >= 4 && buf[3] == '~') { ev.key = Key::PageUp; return ev; } break;
                    case '6': if (len >= 4 && buf[3] == '~') { ev.key = Key::PageDown; return ev; } break;
                    case '2': if (len >= 4 && buf[3] == '~') { ev.key = Key::Insert; return ev; } break;
                    case '3': if (len >= 4 && buf[3] == '~') { ev.key = Key::Delete; return ev; } break;
                    case '1': case '7': case '8': case '9':
                        if (len >= 4 && (buf[3] == '~' || buf[3] == ';')) {
                            // F1-F4
                            if (buf[2] == '1' && buf[3] == '5' && len >= 5 && buf[4] == '~') { ev.key = Key::F5; return ev; }
                            if (buf[2] == '1' && buf[3] == '7' && len >= 5 && buf[4] == '~') { ev.key = Key::F6; return ev; }
                            if (buf[2] == '1' && buf[3] == '8' && len >= 5 && buf[4] == '~') { ev.key = Key::F7; return ev; }
                            if (buf[2] == '1' && buf[3] == '9' && len >= 5 && buf[4] == '~') { ev.key = Key::F8; return ev; }
                            if (buf[2] == '2' && buf[3] == '0' && len >= 5 && buf[4] == '~') { ev.key = Key::F9; return ev; }
                            if (buf[2] == '2' && buf[3] == '1' && len >= 5 && buf[4] == '~') { ev.key = Key::F10; return ev; }
                            if (buf[2] == '2' && buf[3] == '3' && len >= 5 && buf[4] == '~') { ev.key = Key::F11; return ev; }
                            if (buf[2] == '2' && buf[3] == '4' && len >= 5 && buf[4] == '~') { ev.key = Key::F12; return ev; }
                            if (buf[2] == '1' && buf[3] == '~') { // Check for F1-F4 with 1~
                                if (len >= 5) {
                                    if (buf[4] == '5') { ev.key = Key::F5; return ev; }
                                }
                            }
                        }
                        break;
                }
            }
        } else if (len >= 2 && buf[1] >= 'A' && buf[1] <= 'Z') {
            ev.alt = true;
            ev.key = static_cast<Key>(buf[1]);
            return ev;
        } else if (len >= 2 && buf[1] >= 'a' && buf[1] <= 'z') {
            ev.alt = true;
            ev.key = static_cast<Key>(buf[1]);
            return ev;
        }
    }

    // Single byte
    if (len == 1) {
        unsigned char c = static_cast<unsigned char>(buf[0]);
        if (c < 32 && c != 9 && c != 13 && c != 27) {
            // Ctrl+letter
            ev.ctrl = true;
            ev.key = static_cast<Key>(c);
            ev.codepoint = c + 96; // Ctrl+A -> 'a'
            return ev;
        }
        ev.key = static_cast<Key>(c);
        ev.codepoint = c;
        return ev;
    }

    // Multi-byte UTF-8
    if ((buf[0] & 0x80) != 0) {
        ev.key = Key::None;
        // Decode first codepoint
        char32_t cp = 0;
        unsigned char lead = static_cast<unsigned char>(buf[0]);
        if ((lead & 0xE0) == 0xC0 && len >= 2) {
            cp = ((lead & 0x1F) << 6) | (static_cast<unsigned char>(buf[1]) & 0x3F);
        } else if ((lead & 0xF0) == 0xE0 && len >= 3) {
            cp = ((lead & 0x0F) << 12) | ((static_cast<unsigned char>(buf[1]) & 0x3F) << 6) | (static_cast<unsigned char>(buf[2]) & 0x3F);
        } else if ((lead & 0xF8) == 0xF0 && len >= 4) {
            cp = ((lead & 0x07) << 18) | ((static_cast<unsigned char>(buf[1]) & 0x3F) << 12) | ((static_cast<unsigned char>(buf[2]) & 0x3F) << 6) | (static_cast<unsigned char>(buf[3]) & 0x3F);
        }
        ev.codepoint = cp;
        ev.key = Key::None;
        return ev;
    }

    return ev;
}

MouseEvent EventLoop::parseMouse(const char* buf, size_t len) {
    MouseEvent ev{};
    if (len < 6 || buf[0] != '\033' || buf[1] != '[') return ev;

    // Find the end character (M or m)
    size_t end = 0;
    for (size_t i = 2; i < len; ++i) {
        if (buf[i] == 'M' || buf[i] == 'm') { end = i; break; }
    }
    if (end == 0) return ev;

    std::string params(buf + 2, end - 2);
    std::vector<int> vals;
    std::stringstream ss(params);
    std::string item;
    while (std::getline(ss, item, ';')) {
        if (!item.empty()) vals.push_back(std::stoi(item));
    }
    if (vals.size() < 3) return ev;

    int cb = vals[0];
    int cx = vals[1] - 1;
    int cy = vals[2] - 1;

    ev.x = cx;
    ev.y = cy;

    int btn = cb & 0x03;
    if (btn == 0) ev.button = MouseEvent::Left;
    else if (btn == 1) ev.button = MouseEvent::Middle;
    else if (btn == 2) ev.button = MouseEvent::Right;

    if (cb & 0x20) ev.type = MouseEvent::Scroll;
    else if (buf[end] == 'm') ev.type = MouseEvent::Release;
    else ev.type = MouseEvent::Press;

    ev.ctrl = (cb & 0x08) != 0;
    ev.alt = (cb & 0x10) != 0;

    return ev;
}

} // namespace tdesktop
