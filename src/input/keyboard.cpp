#include "keyboard.h"
#include "../utils/logger.h"

namespace tdesktop {

KeyboardHandler::KeyboardHandler() = default;

void KeyboardHandler::handleKey(const KeyEvent& ev) {
    key_state_[ev.key] = true;

    if (keydown_cb_) keydown_cb_(ev);

    // Check hotkeys
    HotkeyKey hk{ev.key, ev.ctrl, ev.alt, ev.shift};
    auto it = hotkeys_.find(hk);
    if (it != hotkeys_.end()) {
        it->second();
    }
}

bool KeyboardHandler::isKeyPressed(Key key) const {
    auto it = key_state_.find(key);
    return it != key_state_.end() && it->second;
}

void KeyboardHandler::bindHotkey(Key key, bool ctrl, bool alt, bool shift, Hotkey action) {
    hotkeys_[{key, ctrl, alt, shift}] = std::move(action);
    LOG_DEBUG("Hotkey bound: ctrl=" + std::to_string(ctrl) +
              " alt=" + std::to_string(alt) +
              " shift=" + std::to_string(shift) +
              " key=" + std::to_string(static_cast<int>(key)));
}

void KeyboardHandler::unbindHotkey(Key key, bool ctrl, bool alt, bool shift) {
    hotkeys_.erase({key, ctrl, alt, shift});
}

} // namespace tdesktop
