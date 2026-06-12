#pragma once
#include <cstdint>
#include <functional>
#include <chrono>
#include <map>

namespace tdesktop {

enum class Key {
    None,
    // ASCII range 0x01-0x1A for Ctrl+A through Ctrl+Z
    CtrlA = 1, CtrlB, CtrlC, CtrlD, CtrlE, CtrlF, CtrlG,
    CtrlH, CtrlI, CtrlJ, CtrlK, CtrlL, CtrlM, CtrlN, CtrlO,
    CtrlP, CtrlQ, CtrlR, CtrlS, CtrlT, CtrlU, CtrlV, CtrlW,
    CtrlX, CtrlY, CtrlZ,
    // Printable ASCII
    Space = ' ', Exclaim = '!', Quote = '"', Hash = '#', Dollar = '$',
    Percent = '%', Ampersand = '&', Apostrophe = '\'', LParen = '(',
    RParen = ')', Asterisk = '*', Plus = '+', Comma = ',', Minus = '-',
    Dot = '.', Slash = '/',
    Digit0 = '0', Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
    Colon = ':', Semicolon = ';', Less = '<', Equal = '=', Greater = '>',
    Question = '?', At = '@',
    A = 'A', B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    LBracket = '[', Backslash = '\\', RBracket = ']', Caret = '^', Underscore = '_',
    Backtick = '`',
    a = 'a', b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
    LBrace = '{', Pipe = '|', RBrace = '}', Tilde = '~',
    Backspace = 127,
    // Special keys
    F1 = 256, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Up, Down, Left, Right,
    Home, End, PageUp, PageDown,
    Insert, Delete,
    Escape = 27,
    Tab = 9,
    Enter = 13,
    Super,
    Alt,
    Shift,
    Ctrl,
};

struct KeyEvent {
    Key key = Key::None;
    char32_t codepoint = 0;
    bool alt = false;
    bool ctrl = false;
    bool shift = false;
};

struct MouseEvent {
    enum Type { Press, Release, Drag, Scroll };
    enum Button { None, Left, Middle, Right };
    Type type = Release;
    Button button = None;
    int x = 0, y = 0;
    bool alt = false, ctrl = false, shift = false;
};

struct Event {
    enum Type { None, Key, Mouse, Resize, Timer, Quit };
    Type type = None;
    KeyEvent key;
    MouseEvent mouse;
    struct { int rows = 0, cols = 0; } resize;
    int timer_id = 0;
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    void init(int stdin_fd);
    bool poll(Event& ev, int timeout_ms = 16);
    void quit() { running_ = false; }
    bool isRunning() const { return running_; }

    int addTimer(int interval_ms, bool oneshot, std::function<void()> callback);
    void removeTimer(int id);

    void onKey(std::function<void(const KeyEvent&)> cb) { keyCallback_ = std::move(cb); }
    void onMouse(std::function<void(const MouseEvent&)> cb) { mouseCallback_ = std::move(cb); }
    void onResize(std::function<void(int, int)> cb) { resizeCallback_ = std::move(cb); }

private:
    int stdin_fd_ = -1;
    bool running_ = false;
    int nextTimerId_ = 1;

    struct TimerInfo {
        int interval_ms;
        bool oneshot;
        std::chrono::steady_clock::time_point next;
        std::function<void()> callback;
    };
    std::map<int, TimerInfo> timers_;

    std::function<void(const KeyEvent&)> keyCallback_;
    std::function<void(const MouseEvent&)> mouseCallback_;
    std::function<void(int, int)> resizeCallback_;

    KeyEvent parseKey(const char* buf, size_t len);
    MouseEvent parseMouse(const char* buf, size_t len);
    bool readInput(Event& ev);
};

} // namespace tdesktop
