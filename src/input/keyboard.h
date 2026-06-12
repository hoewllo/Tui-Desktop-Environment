#pragma once
#include "../core/event.h"
#include <map>
#include <functional>

namespace tdesktop {

class KeyboardHandler {
public:
    KeyboardHandler();
    ~KeyboardHandler() = default;

    void handleKey(const KeyEvent& ev);

    void onKeyDown(std::function<void(const KeyEvent&)> cb) { keydown_cb_ = std::move(cb); }
    void onKeyUp(std::function<void(const KeyEvent&)> cb) { keyup_cb_ = std::move(cb); }

    bool isKeyPressed(Key key) const;

    using Hotkey = std::function<void()>;
    void bindHotkey(Key key, bool ctrl, bool alt, bool shift, Hotkey action);
    void unbindHotkey(Key key, bool ctrl, bool alt, bool shift);

private:
    std::map<Key, bool> key_state_;
    std::function<void(const KeyEvent&)> keydown_cb_;
    std::function<void(const KeyEvent&)> keyup_cb_;

    struct HotkeyKey {
        Key key;
        bool ctrl, alt, shift;
        bool operator<(const HotkeyKey& o) const {
            if (key != o.key) return key < o.key;
            if (ctrl != o.ctrl) return ctrl;
            if (alt != o.alt) return alt;
            return shift < o.shift;
        }
    };
    std::map<HotkeyKey, Hotkey> hotkeys_;
};

} // namespace tdesktop
