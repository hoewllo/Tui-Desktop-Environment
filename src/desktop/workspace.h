#pragma once
#include "../core/buffer.h"
#include "window.h"
#include <string>

namespace tdesktop {

class WorkspaceManager {
public:
    WorkspaceManager(WindowManager& wm);
    ~WorkspaceManager() = default;

    void setCount(int count);
    int getCount() const { return count_; }

    int getCurrent() const { return current_; }
    void setCurrent(int ws);
    void next();
    void prev();

    void moveWindowToCurrent(int window_id);
    void moveWindowToWorkspace(int window_id, int ws);

private:
    WindowManager& wm_;
    int count_ = 4;
    int current_ = 0;
};

} // namespace tdesktop
