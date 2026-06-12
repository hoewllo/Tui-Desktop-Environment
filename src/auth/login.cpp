#include "login.h"
#include "../utils/logger.h"
#include <chrono>
#include <ctime>
#include <memory>

namespace tdesktop {

LoginScreen::LoginScreen(Terminal& term, ColorManager& colors)
    : term_(term), colors_(colors) {
    buffer_ = std::make_unique<Buffer>(term.getCols(), term.getRows());
}

LoginResult LoginScreen::run() {
    running_ = true;
    term_.clearScreen();
    term_.showCursor(true);

    while (running_) {
        draw();

        EventLoop loop;
        loop.init(STDIN_FILENO);

        Event ev;
        while (running_ && loop.poll(ev, 100)) {
            if (ev.type == Event::Key) {
                if (handleKey(ev.key)) {
                    if (input_field_ == 2) {
                        return LoginResult::Success;
                    }
                }
            }
            if (ev.type == Event::Quit) {
                return LoginResult::Quit;
            }
            draw();
        }

        // Handle resize
        TermSize size = term_.getSize();
        if (size.cols != buffer_->getCols() || size.rows != buffer_->getRows()) {
            buffer_->resize(size.cols, size.rows);
        }
    }

    return LoginResult::Quit;
}

void LoginScreen::draw() {
    int cols = term_.getCols();
    int rows = term_.getRows();

    // Background
    colors_.setColorDepth(256);
    for (int y = 0; y < rows; ++y) {
        term_.writeRaw("\033[48;5;17m"); // Dark blue background
        for (int x = 0; x < cols; ++x) {
            term_.writeRaw(" ");
        }
    }
    term_.writeRaw("\033[0m");

    drawBox(cols, rows);

    // Title
    int title_x = cols / 2 - 15;
    int title_y = rows / 2 - 6;
    term_.writeRaw("\033[" + std::to_string(title_y) + ";" + std::to_string(title_x) + "H");
    term_.writeRaw("\033[38;5;46m\033[48;5;17m"); // Green text
    term_.writeRaw("╔══════════════════════════════╗");
    term_.writeRaw("\033[" + std::to_string(title_y + 1) + ";" + std::to_string(title_x) + "H");
    term_.writeRaw("║   TUI Desktop Environment    ║");
    term_.writeRaw("\033[" + std::to_string(title_y + 2) + ";" + std::to_string(title_x) + "H");
    term_.writeRaw("╚══════════════════════════════╝");

    // Username field
    int field_x = cols / 2 - 12;
    int user_y = rows / 2 - 1;
    term_.writeRaw("\033[" + std::to_string(user_y) + ";" + std::to_string(field_x) + "H");
    term_.writeRaw("\033[38;5;15m\033[48;5;17m");
    if (input_field_ == 0) term_.writeRaw("\033[7m");
    term_.writeRaw(" Username: " + username_);
    std::string padding(20 - username_.size(), ' ');
    term_.writeRaw(padding);
    if (input_field_ == 0) term_.writeRaw("\033[27m");

    // Password field
    int pass_y = rows / 2 + 1;
    term_.writeRaw("\033[" + std::to_string(pass_y) + ";" + std::to_string(field_x) + "H");
    term_.writeRaw("\033[38;5;15m\033[48;5;17m");
    if (input_field_ == 1) term_.writeRaw("\033[7m");
    term_.writeRaw(" Password: ");
    std::string masked;
    for (size_t i = 0; i < password_.size(); ++i) masked += "*";
    term_.writeRaw(masked);
    std::string pad2(20 - password_.size(), ' ');
    term_.writeRaw(pad2);
    if (input_field_ == 1) term_.writeRaw("\033[27m");

    // Status/Error message
    if (!error_msg_.empty()) {
        int err_y = rows / 2 + 3;
        term_.writeRaw("\033[" + std::to_string(err_y) + ";" + std::to_string(field_x) + "H");
        term_.writeRaw("\033[38;5;9m\033[48;5;17m"); // Red text
        term_.writeRaw(" " + error_msg_ + "                    ");
        error_msg_.clear();
    }

    // Instructions
    int inst_y = rows / 2 + 5;
    term_.writeRaw("\033[" + std::to_string(inst_y) + ";" + std::to_string(field_x) + "H");
    term_.writeRaw("\033[38;5;8m\033[48;5;17m");
    term_.writeRaw(" Tab: Next field  Enter: Login  ESC: Quit");

    term_.writeRaw("\033[0m");
    term_.writeRaw("\033[" + std::to_string(user_y + (input_field_ == 0 ? 0 : 2)) +
                   ";" + std::to_string(field_x + 11 + cursor_pos_) + "H");
}

bool LoginScreen::handleKey(const KeyEvent& ev) {
    switch (ev.key) {
        case Key::Escape:
            running_ = false;
            return false;
        case Key::Tab:
            input_field_ = (input_field_ + 1) % 2;
            cursor_pos_ = (input_field_ == 0) ? static_cast<int>(username_.size()) : static_cast<int>(password_.size());
            return false;
        case Key::Enter:
            if (input_field_ == 0 && !username_.empty()) {
                input_field_ = 1;
                cursor_pos_ = 0;
            } else if (input_field_ == 1) {
                input_field_ = 2; // Signal to check auth
                return true;
            }
            return false;
        case Key::Backspace:
            if (input_field_ == 0 && !username_.empty()) {
                username_.pop_back();
                cursor_pos_ = static_cast<int>(username_.size());
            } else if (input_field_ == 1 && !password_.empty()) {
                password_.pop_back();
                cursor_pos_ = static_cast<int>(password_.size());
            }
            return false;
        default:
            if (ev.codepoint >= 32 && ev.codepoint < 127) {
                char c = static_cast<char>(ev.codepoint);
                if (input_field_ == 0) {
                    username_ += c;
                    cursor_pos_ = static_cast<int>(username_.size());
                } else if (input_field_ == 1) {
                    password_ += c;
                    cursor_pos_ = static_cast<int>(password_.size());
                }
            }
            return false;
    }
}

void LoginScreen::drawBox(int cols, int rows) {
    // Draw centered box using raw escape codes
    int box_w = 44;
    int box_h = 14;
    int box_x = cols / 2 - box_w / 2;
    int box_y = rows / 2 - box_h / 2;

    term_.writeRaw("\033[38;5;8m\033[48;5;17m");
    for (int y = box_y; y < box_y + box_h; ++y) {
        term_.writeRaw("\033[" + std::to_string(y) + ";" + std::to_string(box_x) + "H");
        for (int x = 0; x < box_w; ++x) {
            if (y == box_y || y == box_y + box_h - 1) {
                if (x == 0) term_.writeRaw(y == box_y ? "╔" : "╚");
                else if (x == box_w - 1) term_.writeRaw(y == box_y ? "╗" : "╝");
                else term_.writeRaw("═");
            } else {
                if (x == 0 || x == box_w - 1) term_.writeRaw("║");
                else term_.writeRaw(" ");
            }
        }
    }
}

} // namespace tdesktop
