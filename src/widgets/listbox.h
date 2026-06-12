#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "../core/event.h"
#include <string>
#include <vector>
#include <functional>

namespace tdesktop {

class ListBox {
public:
    ListBox(int x, int y, int w, int h);
    ~ListBox() = default;

    void draw(Renderer& r);
    bool handleKey(const KeyEvent& ev);
    bool handleClick(int mx, int my);

    void addItem(const std::string& item);
    void removeItem(int idx);
    void clearItems();
    void setItems(const std::vector<std::string>& items);

    int getSelected() const { return selected_; }
    void setSelected(int idx);
    std::string getSelectedItem() const;

    void setFocused(bool f) { focused_ = f; }
    bool isFocused() const { return focused_; }

    int getCount() const { return static_cast<int>(items_.size()); }

    void onSelect(std::function<void(int, const std::string&)> cb) { select_cb_ = std::move(cb); }

private:
    int x_, y_, w_, h_;
    std::vector<std::string> items_;
    int selected_ = 0;
    int scroll_ = 0;
    bool focused_ = false;
    std::function<void(int, const std::string&)> select_cb_;
};

} // namespace tdesktop
