#include "mouse.h"
#include "../utils/logger.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace tdesktop {

MouseHandler::MouseHandler() = default;

MouseHandler::~MouseHandler() {
    disableTerminalProtocol();
    closeHardwareDevice();
}

bool MouseHandler::init() {
    mode_ = detectMode();
    if (mode_ == Hardware) {
        enableHardwareMouse();
        if (hw_fd_ < 0) {
            enableTerminalProtocol();
            mode_ = TerminalProtocol;
        }
    } else if (mode_ == TerminalProtocol) {
        enableTerminalProtocol();
    }
    LOG_INFO("Mouse mode: " + std::to_string(static_cast<int>(mode_)));
    return true;
}

void MouseHandler::enableTerminalProtocol() {
    // Enable SGR mouse mode (1006) + button events + motion
    ::write(STDOUT_FILENO, "\033[?1000h", 8);  // Button events
    ::write(STDOUT_FILENO, "\033[?1002h", 8);  // Button drag
    ::write(STDOUT_FILENO, "\033[?1006h", 8);  // SGR coordinates
    ::write(STDOUT_FILENO, "\033[?1005h", 8);  // UTF-8 mode (extended)
}

void MouseHandler::disableTerminalProtocol() {
    ::write(STDOUT_FILENO, "\033[?1000l", 8);
    ::write(STDOUT_FILENO, "\033[?1002l", 8);
    ::write(STDOUT_FILENO, "\033[?1006l", 8);
    ::write(STDOUT_FILENO, "\033[?1005l", 8);
}

void MouseHandler::setMode(Mode mode) {
    if (mode == mode_) return;
    disableTerminalProtocol();
    closeHardwareDevice();
    mode_ = mode;
    if (mode_ == TerminalProtocol) enableTerminalProtocol();
    else if (mode_ == Hardware) enableHardwareMouse();
}

MouseHandler::Mode MouseHandler::detectMode() const {
    // Try hardware mouse first
    int fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (fd >= 0) {
        close(fd);
        return Hardware;
    }
    return TerminalProtocol;
}

void MouseHandler::handleMouse(const MouseEvent& ev) {
    x_ = ev.x;
    y_ = ev.y;

    switch (ev.type) {
        case MouseEvent::Press:
            if (click_cb_) click_cb_(ev);
            break;
        case MouseEvent::Drag:
            if (drag_cb_) drag_cb_(ev);
            break;
        case MouseEvent::Scroll:
            if (scroll_cb_) scroll_cb_(ev);
            break;
        default:
            break;
    }
}

bool MouseHandler::openHardwareDevice() {
    hw_fd_ = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (hw_fd_ < 0) {
        LOG_WARN("Failed to open /dev/input/mice: " + std::string(strerror(errno)));
        return false;
    }
    LOG_INFO("Hardware mouse device opened");
    return true;
}

void MouseHandler::closeHardwareDevice() {
    if (hw_fd_ >= 0) {
        close(hw_fd_);
        hw_fd_ = -1;
    }
}

void MouseHandler::enableHardwareMouse() {
    openHardwareDevice();
}

void MouseHandler::disableHardwareMouse() {
    closeHardwareDevice();
}

bool MouseHandler::readHardwareMouse(MouseEvent& ev) {
    if (hw_fd_ < 0) return false;
    unsigned char data[3];
    ssize_t n = read(hw_fd_, data, 3);
    if (n < 3) return false;

    ev.type = MouseEvent::Drag;
    ev.button = MouseEvent::None;
    if (data[0] & 0x01) ev.button = MouseEvent::Left;
    if (data[0] & 0x02) ev.button = MouseEvent::Right;
    if (data[0] & 0x04) ev.button = MouseEvent::Middle;

    // Relative movement - accumulate
    static int acc_x = 0, acc_y = 0;
    acc_x += static_cast<int>(static_cast<signed char>(data[1]));
    acc_y -= static_cast<int>(static_cast<signed char>(data[2]));

    ev.x = acc_x;
    ev.y = acc_y;

    return true;
}

} // namespace tdesktop
