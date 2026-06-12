#pragma once
#include "../core/terminal.h"
#include "../core/buffer.h"
#include "../core/event.h"
#include "../core/renderer.h"
#include "../core/color.h"
#include "../input/keyboard.h"
#include "../input/mouse.h"
#include "window.h"
#include "taskbar.h"
#include "launcher.h"
#include "workspace.h"
#include "command_panel.h"
#include <memory>
#include <functional>

namespace tdesktop {

class Desktop {
public:
    Desktop(Terminal& term, EventLoop& loop, ColorManager& colors);
    ~Desktop();

    Desktop(const Desktop&) = delete;
    Desktop& operator=(const Desktop&) = delete;

    bool init();
    void run();
    void shutdown();

    void render();
    void handleEvent(const Event& ev);
    void handleKeyEvent(const KeyEvent& ev);
    void handleMouseEvent(const MouseEvent& ev);

    WindowManager& getWindowManager() { return wm_; }
    Taskbar& getTaskbar() { return taskbar_; }
    Launcher& getLauncher() { return launcher_; }
    WorkspaceManager& getWorkspaceManager() { return ws_mgr_; }
    CommandPanel& getCommandPanel() { return cmd_panel_; }
    KeyboardHandler& getKeyboard() { return keyboard_; }
    MouseHandler& getMouse() { return mouse_; }

private:
    Terminal& term_;
    EventLoop& loop_;
    ColorManager& colors_;

    std::unique_ptr<Buffer> front_buffer_;
    std::unique_ptr<Buffer> back_buffer_;
    std::unique_ptr<Renderer> renderer_;

    WindowManager wm_;
    Taskbar taskbar_;
    Launcher launcher_;
    WorkspaceManager ws_mgr_;
    CommandPanel cmd_panel_;
    KeyboardHandler keyboard_;
    MouseHandler mouse_;

    bool running_ = false;
    bool help_visible_ = false;
    int fps_timer_ = 0;

    void setupHotkeys();
    void drawDesktopBackground();
    void drawHelpPanel();
    void drawClock();
    void updateClock();
};

} // namespace tdesktop
