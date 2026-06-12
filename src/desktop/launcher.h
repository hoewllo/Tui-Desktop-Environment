#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "../widgets/textbox.h"
#include "../widgets/listbox.h"
#include <string>
#include <vector>
#include <functional>

namespace tdesktop {

struct AppEntry {
    std::string name;
    std::string exec;
    std::string comment;
    std::string icon;
    std::string categories;
};

class Launcher {
public:
    Launcher();
    ~Launcher() = default;

    void scanApplications();
    void scanDirectory(const std::string& path);
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    void toggle() { visible_ = !visible_; }

    void draw(Renderer& r, int cols, int rows);
    bool handleKey(const KeyEvent& ev);
    bool handleClick(int x, int y);

    void onLaunch(std::function<void(const std::string&)> cb) { launch_cb_ = std::move(cb); }
    std::function<void(const std::string&)> launch_cb_;

    const std::vector<AppEntry>& getApps() const { return apps_; }

private:
    std::vector<AppEntry> apps_;
    std::vector<AppEntry> filtered_;
    bool visible_ = false;
    int selected_idx_ = 0;
    std::string search_text_;

    void filterApps();
    void launchSelected();
};

} // namespace tdesktop
