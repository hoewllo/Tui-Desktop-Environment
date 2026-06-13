#pragma once
#include "../core/terminal.h"
#include "../core/event.h"
#include "../core/renderer.h"
#include "../core/color.h"
#include "../utils/registry.h"
#include <string>
#include <vector>
#include <memory>

namespace tdesktop {

class RegEditor {
public:
    RegEditor(Terminal& term, ColorManager& colors);
    ~RegEditor() = default;

    void show();
    void hide();
    bool isVisible() const { return visible_; }

    void draw(Renderer& r);
    bool handleKey(const KeyEvent& ev);

private:
    Terminal& term_;
    ColorManager& colors_;
    bool visible_ = false;

    std::vector<std::string> key_path_;
    std::vector<std::string> subkeys_;
    std::vector<std::string> values_;
    int selected_ = 0;
    bool focus_values_ = false;

    void refreshKeys();
    void drawTree(Renderer& r, int x, int y, int w, int h);
    void drawValues(Renderer& r, int x, int y, int w, int h);
    std::string currentKey() const;
};

} // namespace tdesktop
