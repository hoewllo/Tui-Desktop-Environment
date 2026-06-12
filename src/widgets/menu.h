#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "../core/event.h"
#include <string>
#include <vector>
#include <functional>

namespace tdesktop {

struct MenuItem {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    bool separator = false;
    std::function<void()> action;
};

class Menu {
public:
    Menu();
    ~Menu() = default;

    void addItem(const std::string& label, std::function<void()> action, const std::string& shortcut = "");
    void addSeparator();
    void clear();

    void show(int x, int y);
    void hide();
    bool isVisible() const { return visible_; }

    void draw(Renderer& r);
    bool handleKey(const KeyEvent& ev);
    bool handleClick(int mx, int my);

    int getWidth() const;
    int getHeight() const;

private:
    std::vector<MenuItem> items_;
    int selected_ = 0;
    int x_ = 0, y_ = 0;
    bool visible_ = false;
};

} // namespace tdesktop
