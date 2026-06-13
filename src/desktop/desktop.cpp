#include "desktop.h"
#include "../utils/logger.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>

namespace tdesktop {

Desktop::Desktop(Terminal& term, EventLoop& loop, ColorManager& colors)
    : term_(term), loop_(loop), colors_(colors),
      taskbar_(wm_), ws_mgr_(wm_),
      reg_editor_(term, colors) {
}

Desktop::~Desktop() {
    shutdown();
}

bool Desktop::init() {
    int cols = term_.getCols();
    int rows = term_.getRows();

    front_buffer_ = std::make_unique<Buffer>(cols, rows);
    back_buffer_ = std::make_unique<Buffer>(cols, rows);
    renderer_ = std::make_unique<Renderer>(*front_buffer_, *back_buffer_, colors_);

    mouse_.init();
    cmd_panel_.init();
    setupContextMenu();

    // Handle resize
    term_.onResize([this](TermSize size) {
        front_buffer_->resize(size.cols, size.rows);
        back_buffer_->resize(size.cols, size.rows);
    });

    setupHotkeys();
    updateClock();
    LOG_INFO("Desktop initialized");
    return true;
}

void Desktop::run() {
    running_ = true;

    // FPS timer
    fps_timer_ = loop_.addTimer(1000, false, [this]() {
        // FPS update handled by renderer
    });

    // Clock update every 10 seconds
    loop_.addTimer(10000, false, [this]() { updateClock(); });

    // Setup keyboard handler
    loop_.onKey([this](const KeyEvent& ev) { handleKeyEvent(ev); });
    loop_.onMouse([this](const MouseEvent& ev) { handleMouseEvent(ev); });
    loop_.onResize([this](int rows, int cols) {
        front_buffer_->resize(cols, rows);
        back_buffer_->resize(cols, rows);
    });

    render();

    while (running_ && loop_.isRunning()) {
        Event ev;
        if (loop_.poll(ev, 16)) {
            switch (ev.type) {
                case Event::Key:
                    handleKeyEvent(ev.key);
                    break;
                case Event::Mouse:
                    handleMouseEvent(ev.mouse);
                    break;
                case Event::Quit:
                    running_ = false;
                    break;
                default:
                    break;
            }
        }
        render();
    }
}

void Desktop::shutdown() {
    running_ = false;
    mouse_.disableTerminalProtocol();
}

void Desktop::render() {
    int cols = term_.getCols();
    int rows = term_.getRows();

    renderer_->clear();
    drawDesktopBackground();
    drawClock();

    if (help_visible_) {
        drawHelpPanel();
    } else if (launcher_.isVisible()) {
        launcher_.draw(*renderer_, cols, rows);
    } else if (cmd_panel_.isOpen()) {
        // Draw windows compressed
        int cmd_h = cmd_panel_.getHeight();
        drawDesktopBackground();
        auto windows = wm_.getWindowsOnWorkspace(wm_.getCurrentWorkspace());
        for (auto* win : windows) {
            // Scale down window positions
            int wy = win->y * (rows - cmd_h) / rows;
            int wh = win->h * (rows - cmd_h) / rows;
            // Draw compressed window
            renderer_->setFGIdx(7);
            renderer_->setBGIdx(4);
            renderer_->drawText(win->x, wy, " " + win->title + " ");
            renderer_->drawRectOutline({win->x, wy, win->w, wh});
            // Draw close button
            renderer_->setFGIdx(9);
            renderer_->drawText(win->x + win->w - 4, wy, "[X]");
        }
        taskbar_.draw(*renderer_, cols);
        cmd_panel_.draw(*renderer_, cols, rows);
    } else {
        auto windows = wm_.getWindowsOnWorkspace(wm_.getCurrentWorkspace());
        for (auto* win : windows) {
            // Title bar
            renderer_->setBGIdx(4);
            renderer_->setFGIdx(15);
            if (win->focused) {
                renderer_->setBGIdx(4);
                renderer_->setFGIdx(15);
            } else {
                renderer_->setBGIdx(8);
                renderer_->setFGIdx(7);
            }
            for (int x = win->x; x < win->x + win->w; ++x) {
                renderer_->drawChar(x, win->y, L' ');
            }
            std::string title = " " + win->title + " ";
            renderer_->drawText(win->x + 1, win->y, title);

            // Close button
            renderer_->setFGIdx(9);
            renderer_->drawText(win->x + win->w - 4, win->y, "[X]");

            // Window border
            renderer_->setFGIdx(7);
            renderer_->setBGIdx(0);
            renderer_->drawRectOutline(win->rect());

            // Window content area
            for (int wy = win->y + 1; wy < win->y + win->h - 1; ++wy) {
                for (int wx = win->x + 1; wx < win->x + win->w - 1; ++wx) {
                    renderer_->drawChar(wx, wy, L' ');
                }
            }
        }
        taskbar_.draw(*renderer_, cols);
    }

    // FPS display
    if (renderer_->getFPS() > 0) {
        renderer_->setFGIdx(8);
        std::string fps = "FPS: " + std::to_string(static_cast<int>(renderer_->getFPS()));
        renderer_->drawText(2, rows - 1, fps);
    }

    // Context menu (topmost)
    if (ctx_menu_.isVisible()) {
        ctx_menu_.draw(*renderer_);
    }

    // Registry editor (topmost)
    if (reg_editor_.isVisible()) {
        reg_editor_.draw(*renderer_);
    }

    renderer_->present();
}

void Desktop::handleKeyEvent(const KeyEvent& ev) {
    // Registry editor gets highest priority
    if (reg_editor_.isVisible()) {
        reg_editor_.handleKey(ev);
        return;
    }

    // Context menu gets priority when open
    if (ctx_menu_.isVisible()) {
        ctx_menu_.handleKey(ev);
        return;
    }

    // Command panel gets priority when open
    if (cmd_panel_.isOpen()) {
        cmd_panel_.handleKey(ev);
        return;
    }

    // Launcher gets priority when open
    if (launcher_.isVisible()) {
        launcher_.handleKey(ev);
        return;
    }

    keyboard_.handleKey(ev);
}

void Desktop::handleMouseEvent(const MouseEvent& ev) {
    mouse_.handleMouse(ev);

    // Registry editor swallows all mouse when open
    if (reg_editor_.isVisible()) {
        return;
    }

    // Context menu gets priority when open
    if (ctx_menu_.isVisible()) {
        ctx_menu_.handleClick(ev.x, ev.y);
        return;
    }

    if (launcher_.isVisible()) {
        launcher_.handleClick(ev.x, ev.y);
        return;
    }

    if (cmd_panel_.isOpen()) return;

    // Check taskbar click
    if (taskbar_.handleClick(ev.x, ev.y)) return;

    // Right-click on desktop opens context menu
    if (ev.button == MouseEvent::Right && ev.type == MouseEvent::Press) {
        ctx_menu_.show(ev.x, ev.y);
        return;
    }

    // Check window click
    auto* win = wm_.getWindowAt(ev.x, ev.y);
    if (win) {
        if (ev.type == MouseEvent::Press) {
            wm_.focusWindow(win->id);
            if (win->closeButtonContains(ev.x, ev.y)) {
                wm_.closeWindow(win->id);
                return;
            }
            if (win->titleBarContains(ev.x, ev.y)) {
                // Start drag - handle in drag event
            }
        }
    }
}

void Desktop::setupHotkeys() {
    keyboard_.bindHotkey(Key::F12, false, false, false, [this]() {
        cmd_panel_.toggle();
    });

    keyboard_.bindHotkey(Key::Backtick, true, false, false, [this]() {
        cmd_panel_.toggle();
    });

    keyboard_.bindHotkey(Key::q, true, false, false, [this]() {
        running_ = false;
        loop_.quit();
    });

    keyboard_.bindHotkey(Key::l, true, false, false, [this]() {
        term_.clearScreen();
    });

    keyboard_.bindHotkey(Key::r, true, false, false, [this]() {
        // Relayout windows
    });

    keyboard_.bindHotkey(Key::h, true, false, false, [this]() {
        help_visible_ = !help_visible_;
    });

    keyboard_.bindHotkey(Key::Tab, true, false, false, [this]() {
        wm_.focusNext();
    });

    keyboard_.bindHotkey(Key::F4, true, false, false, [this]() {
        wm_.closeFocusedWindow();
    });

    keyboard_.bindHotkey(Key::Escape, false, false, false, [this]() {
        if (help_visible_) {
            help_visible_ = false;
        } else if (launcher_.isVisible()) {
            launcher_.setVisible(false);
        }
    });

    // Workspace switching: Alt+1..9
    auto bindWs = [this](int ws) {
        keyboard_.bindHotkey(static_cast<Key>(static_cast<int>(Key::Digit1) + ws),
            false, true, false, [this, ws]() { ws_mgr_.setCurrent(ws); });
    };
    for (int i = 0; i < 9; ++i) bindWs(i);

    // Super/Win key - open launcher
    keyboard_.bindHotkey(Key::Super, false, false, false, [this]() {
        launcher_.toggle();
    });
}

void Desktop::drawDesktopBackground() {
    int cols = term_.getCols();
    int rows = term_.getRows();

    // Default background
    renderer_->setBGIdx(0);
    renderer_->setFGIdx(7);

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            renderer_->drawChar(x, y, L' ');
        }
    }
}

void Desktop::drawHelpPanel() {
    int cols = term_.getCols();
    int rows = term_.getRows();
    int box_w = std::min(70, cols - 4);
    int box_h = std::min(28, rows - 4);
    int box_x = (cols - box_w) / 2;
    int box_y = (rows - box_h) / 2;

    renderer_->setBGIdx(0);
    renderer_->setFGIdx(7);
    renderer_->drawRectOutline({box_x, box_y, box_w, box_h});

    // Title
    renderer_->setFGIdx(15);
    renderer_->setBGIdx(4);
    renderer_->drawText(box_x + 2, box_y + 1, " TDESKTOP Help [ESC] Close ");

    // Help content
    std::vector<std::string> help_lines = {
        "",
        "  Global Hotkeys:",
        "    F12 / Ctrl+`    Toggle command panel",
        "    Ctrl+Q          Quit desktop",
        "    Ctrl+L          Refresh screen",
        "    Ctrl+R          Relayout windows",
        "    Ctrl+H          Toggle help",
        "",
        "  Window Management:",
        "    Alt+Tab         Cycle window focus",
        "    Alt+F4          Close focused window",
        "    Alt+1..9        Switch workspace",
        "",
        "  Mouse:",
        "    Left click      Activate window/button",
        "    Title bar drag  Move window",
        "    [X] button      Close window",
        "",
        "  Launcher:",
        "    Super/Win key   Open launcher",
        "    Type to search  Filter apps",
        "    Enter to launch Selected app",
    };

    int ly = box_y + 3;
    renderer_->setFGIdx(7);
    renderer_->setBGIdx(0);
    for (const auto& line : help_lines) {
        if (ly >= box_y + box_h - 2) break;
        renderer_->drawText(box_x + 1, ly++, line);
    }
}

void Desktop::drawClock() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
    taskbar_.setClock(std::string(buf));
}

void Desktop::updateClock() {
    drawClock();
}

void Desktop::setupContextMenu() {
    ctx_menu_.addItem("Terminal", [this]() {
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            const char* shell = getenv("SHELL");
            if (!shell) shell = "/bin/bash";
            execl(shell, shell, nullptr);
            _exit(1);
        }
    }, "Ctrl+T");

    ctx_menu_.addItem("Launcher", [this]() {
        launcher_.toggle();
    }, "Super");

    ctx_menu_.addSeparator();

    ctx_menu_.addItem("Help", [this]() {
        help_visible_ = true;
    }, "Ctrl+H");

    ctx_menu_.addItem("Command Panel", [this]() {
        cmd_panel_.toggle();
    }, "F12");

    ctx_menu_.addSeparator();

    ctx_menu_.addItem("Registry Editor", [this]() {
        reg_editor_.show();
    }, "");

    ctx_menu_.addItem("Quit", [this]() {
        running_ = false;
        loop_.quit();
    }, "Ctrl+Q");
}

} // namespace tdesktop
