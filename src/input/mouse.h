#pragma once
#include "../core/event.h"
#include <functional>

namespace tdesktop {

class MouseHandler {
public:
    enum Mode { Hardware, TerminalProtocol, None };

    MouseHandler();
    ~MouseHandler();

    MouseHandler(const MouseHandler&) = delete;
    MouseHandler& operator=(const MouseHandler&) = delete;

    bool init();
    void enableTerminalProtocol();
    void disableTerminalProtocol();
    void setMode(Mode mode);
    Mode getMode() const { return mode_; }
    Mode detectMode() const;

    void handleMouse(const MouseEvent& ev);

    void onClick(std::function<void(const MouseEvent&)> cb) { click_cb_ = std::move(cb); }
    void onDrag(std::function<void(const MouseEvent&)> cb) { drag_cb_ = std::move(cb); }
    void onScroll(std::function<void(const MouseEvent&)> cb) { scroll_cb_ = std::move(cb); }

    void enableHardwareMouse();
    void disableHardwareMouse();
    bool readHardwareMouse(MouseEvent& ev);

    int getX() const { return x_; }
    int getY() const { return y_; }

private:
    Mode mode_ = None;
    int x_ = 0, y_ = 0;
    int hw_fd_ = -1;

    std::function<void(const MouseEvent&)> click_cb_;
    std::function<void(const MouseEvent&)> drag_cb_;
    std::function<void(const MouseEvent&)> scroll_cb_;

    bool openHardwareDevice();
    void closeHardwareDevice();
};

} // namespace tdesktop
