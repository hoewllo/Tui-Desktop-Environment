#pragma once
#include "../core/buffer.h"
#include <string>
#include <cstdint>
#include <memory>
#include <vector>

namespace tdesktop {

struct Window {
    int id = 0;
    std::string title;
    int x = 0, y = 0, w = 60, h = 20;
    int min_w = 20, min_h = 5;
    bool visible = true;
    bool focused = false;
    bool closable = true;
    bool movable = true;
    bool resizable = true;
    int workspace = 0;
    int z_order = 0;

    enum class State { Normal, Minimized, Maximized };
    State state = State::Normal;

    // Content buffer
    std::unique_ptr<Buffer> content;

    Window() = default;

    Rect rect() const { return {x, y, w, h}; }
    Rect clientRect() const { return {x + 1, y + 1, w - 2, h - 2}; }

    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool titleBarContains(int px, int py) const {
        return px >= x && px < x + w && py == y;
    }

    bool closeButtonContains(int px, int py) const {
        return px >= x + w - 4 && px < x + w - 1 && py == y;
    }
};

class WindowManager {
public:
    WindowManager();
    ~WindowManager() = default;

    int createWindow(const std::string& title, int w, int h);
    void closeWindow(int id);
    void closeFocusedWindow();

    Window* getWindow(int id);
    Window* getFocusedWindow();
    Window* getWindowAt(int x, int y);
    const std::vector<std::unique_ptr<Window>>& getWindows() const { return windows_; }

    void focusWindow(int id);
    void focusNext();
    void focusPrev();

    void moveWindow(int id, int x, int y);
    void resizeWindow(int id, int w, int h);
    void setWindowTitle(int id, const std::string& title);

    void moveWindowToWorkspace(int id, int ws);
    std::vector<Window*> getWindowsOnWorkspace(int ws) const;

    void raiseWindow(int id);
    void setWorkspaceCount(int count) { workspace_count_ = count; }
    int getWorkspaceCount() const { return workspace_count_; }
    int getCurrentWorkspace() const { return current_workspace_; }
    void setCurrentWorkspace(int ws);

    void clear();

private:
    std::vector<std::unique_ptr<Window>> windows_;
    int next_id_ = 1;
    int max_z_ = 0;
    int workspace_count_ = 4;
    int current_workspace_ = 0;

    void updateZOrder();
};

} // namespace tdesktop
