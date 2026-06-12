#include "command_panel.h"
#include "../utils/logger.h"
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <cstring>
#include <sstream>
#include <signal.h>

#ifdef __linux__
#include <pty.h>
#else
#include <util.h>
#endif

namespace tdesktop {

CommandPanel::CommandPanel() = default;

CommandPanel::~CommandPanel() {
    killShell();
}

void CommandPanel::init() {
    spawnShell();
}

void CommandPanel::toggle() {
    if (open_) close();
    else open();
}

void CommandPanel::open() {
    open_ = true;
    if (!hasShell()) spawnShell();
}

void CommandPanel::close() {
    open_ = false;
}

void CommandPanel::draw(Renderer& r, int cols, int rows) {
    if (!open_) return;

    int panel_y = rows - height_;
    input_row_ = rows - 2;

    // Panel background
    r.setBGIdx(0);
    r.setFGIdx(7);

    // Draw panel area
    for (int y = panel_y; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            r.drawChar(x, y, L' ');
        }
    }

    // Draw divider
    r.setFGIdx(8);
    for (int x = 0; x < cols; ++x) {
        r.drawChar(x, panel_y, L'─');
    }

    // Draw output
    int out_rows = height_ - 3;
    int start_line = std::max(0, static_cast<int>(output_lines_.size()) - out_rows - scroll_offset_);
    for (int i = 0; i < out_rows && start_line + i < static_cast<int>(output_lines_.size()); ++i) {
        std::string line = output_lines_[start_line + i];
        if (static_cast<int>(line.size()) > cols) line = line.substr(0, cols);
        r.setFGIdx(7);
        r.drawText(0, panel_y + 1 + i, line);
    }

    // Draw input line
    r.setFGIdx(10); // Green prompt
    r.drawText(0, input_row_, "$ ");
    r.setFGIdx(15);
    r.drawText(2, input_row_, input_buffer_);

    // Draw cursor
    if (open_) {
        r.setFGIdx(15);
        r.setBGIdx(15);
        r.drawChar(2 + cursor_pos_, input_row_, L' ');
        r.setBGIdx(0);
    }
}

bool CommandPanel::handleKey(const KeyEvent& ev) {
    if (!open_) return false;

    switch (ev.key) {
        case Key::Escape:
            close();
            return true;
        case Key::Enter:
            executeCommand();
            return true;
        case Key::Backspace:
            if (cursor_pos_ > 0 && !input_buffer_.empty()) {
                input_buffer_.erase(cursor_pos_ - 1, 1);
                cursor_pos_--;
            }
            return true;
        case Key::Up:
            if (!history_.empty()) {
                if (history_pos_ < 0) history_pos_ = static_cast<int>(history_.size()) - 1;
                else if (history_pos_ > 0) history_pos_--;
                input_buffer_ = history_[history_pos_];
                cursor_pos_ = static_cast<int>(input_buffer_.size());
            }
            return true;
        case Key::Down:
            if (history_pos_ >= 0) {
                history_pos_++;
                if (history_pos_ >= static_cast<int>(history_.size())) {
                    history_pos_ = -1;
                    input_buffer_.clear();
                } else {
                    input_buffer_ = history_[history_pos_];
                }
                cursor_pos_ = static_cast<int>(input_buffer_.size());
            }
            return true;
        case Key::Left:
            if (cursor_pos_ > 0) cursor_pos_--;
            return true;
        case Key::Right:
            if (cursor_pos_ < static_cast<int>(input_buffer_.size())) cursor_pos_++;
            return true;
        case Key::Home:
            cursor_pos_ = 0;
            return true;
        case Key::End:
            cursor_pos_ = static_cast<int>(input_buffer_.size());
            return true;
        default:
            if (ev.ctrl && ev.key == Key::u) {
                input_buffer_.clear();
                cursor_pos_ = 0;
                return true;
            }
            if (ev.ctrl && ev.key == Key::l) {
                output_lines_.clear();
                return true;
            }
            if (ev.key == Key::Tab) {
                return true;
            }
            if (ev.codepoint >= 32 && ev.codepoint < 127) {
                input_buffer_.insert(cursor_pos_, 1, static_cast<char>(ev.codepoint));
                cursor_pos_++;
                return true;
            }
            break;
    }
    return false;
}

void CommandPanel::executeCommand() {
    std::string cmd = input_buffer_;
    if (cmd.empty()) return;

    addToHistory(cmd);
    input_buffer_.clear();
    cursor_pos_ = 0;
    history_pos_ = -1;

    if (cmd_cb_) cmd_cb_(cmd);

    if (hasShell()) {
        writeInput(cmd + "\n");
    } else {
        // Fallback: use popen
        FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
        if (!pipe) {
            output_lines_.push_back("Error: failed to execute command");
            return;
        }
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) {
            std::string line(buf);
            if (!line.empty() && line.back() == '\n') line.pop_back();
            output_lines_.push_back(line);
            if (output_lines_.size() > 1000) output_lines_.erase(output_lines_.begin());
        }
        pclose(pipe);
    }
}

void CommandPanel::addToHistory(const std::string& cmd) {
    if (history_.empty() || history_.back() != cmd) {
        history_.push_back(cmd);
        if (history_.size() > 500) history_.pop_front();
    }
}

void CommandPanel::spawnShell() {
    int master_fd;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);
    if (pid < 0) {
        LOG_ERROR("forkpty failed");
        return;
    }
    if (pid == 0) {
        const char* shell = getenv("SHELL");
        if (!shell) shell = "/bin/bash";
        execl(shell, shell, nullptr);
        _exit(1);
    }
    pid_ = pid;
    master_fd_ = master_fd;
    LOG_INFO("Shell spawned: pid=" + std::to_string(pid));
}

void CommandPanel::killShell() {
    if (pid_ > 0) {
        kill(pid_, SIGTERM);
        waitpid(pid_, nullptr, WNOHANG);
        pid_ = -1;
    }
    if (master_fd_ >= 0) {
        ::close(master_fd_);
        master_fd_ = -1;
    }
}

void CommandPanel::readOutput() {
    if (master_fd_ < 0) return;
    char buf[4096];
    ssize_t n = read(master_fd_, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::string output(buf);
        std::stringstream ss(output);
        std::string line;
        while (std::getline(ss, line, '\n')) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            output_lines_.push_back(line);
            if (output_lines_.size() > 1000) output_lines_.erase(output_lines_.begin());
        }
    }
}

void CommandPanel::writeInput(const std::string& data) {
    if (master_fd_ >= 0) {
        ::write(master_fd_, data.data(), data.size());
    }
}

void CommandPanel::clearOutput() {
    output_lines_.clear();
}

} // namespace tdesktop
