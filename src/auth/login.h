#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "../core/event.h"
#include "../core/color.h"
#include "../core/terminal.h"
#include <string>
#include <memory>
#include <functional>

namespace tdesktop {

enum class LoginResult {
    Success,
    Failed,
    Quit
};

class LoginScreen {
public:
    LoginScreen(Terminal& term, ColorManager& colors);
    ~LoginScreen() = default;

    LoginResult run();
    void draw();
    bool handleKey(const KeyEvent& ev);

    std::string getUsername() const { return username_; }
    std::string getPassword() const { return password_; }

    void setError(const std::string& msg) { error_msg_ = msg; }
    void setAttempts(int a) { attempts_ = a; max_attempts_ = a; }
    int getAttempts() const { return attempts_; }

private:
    Terminal& term_;
    ColorManager& colors_;
    std::unique_ptr<Buffer> buffer_;

    std::string username_;
    std::string password_;
    std::string error_msg_;
    std::string status_msg_;
    bool show_password_ = false;
    int cursor_pos_ = 0;
    int input_field_ = 0; // 0=username, 1=password
    int attempts_ = 0;
    int max_attempts_ = 3;
    bool running_ = false;

    void drawBox(int cols, int rows);
};

} // namespace tdesktop
