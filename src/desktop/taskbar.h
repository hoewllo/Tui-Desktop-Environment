#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "window.h"
#include <string>
#include <functional>

namespace tdesktop {

class Taskbar {
public:
    Taskbar(WindowManager& wm);
    ~Taskbar() = default;

    void draw(Renderer& r, int cols);
    bool handleClick(int x, int y);

    void onWindowClick(std::function<void(int)> cb) { window_click_cb_ = std::move(cb); }

    void setClock(const std::string& time) { clock_ = time; }
    std::string getClock() const { return clock_; }

private:
    WindowManager& wm_;
    std::string clock_;
    std::function<void(int)> window_click_cb_;
    int taskbar_height_ = 1;
};

} // namespace tdesktop
