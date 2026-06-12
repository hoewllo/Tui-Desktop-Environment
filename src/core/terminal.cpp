#include "terminal.h"
#include "../utils/logger.h"
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <csignal>

namespace tdesktop {

Terminal* Terminal::instance_ = nullptr;

Terminal::Terminal() {
    instance_ = this;
}

Terminal::~Terminal() {
    restore();
}

bool Terminal::init() {
    if (!isatty(STDIN_FILENO)) {
        LOG_ERROR("stdin is not a tty");
        return false;
    }
    saved_stdout_ = dup(STDOUT_FILENO);
    detectSize();
    enableRawMode();
    enterAlternateScreen();
    handleSigwinch();
    initialized_ = true;
    LOG_INFO("Terminal initialized: " + std::to_string(size_.cols) + "x" + std::to_string(size_.rows));
    return true;
}

void Terminal::restore() {
    if (!initialized_) return;
    initialized_ = false;
    showCursor(true);
    exitAlternateScreen();
    disableRawMode();
    signal(SIGWINCH, SIG_DFL);
    LOG_INFO("Terminal restored");
}

void Terminal::enableRawMode() {
    termios raw{};
    tcgetattr(STDIN_FILENO, &original_termios_);
    raw = original_termios_;
    raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void Terminal::disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios_);
}

void Terminal::enterAlternateScreen() {
    writeRaw("\033[?1049h");
}

void Terminal::exitAlternateScreen() {
    writeRaw("\033[?1049l");
}

void Terminal::showCursor(bool show) {
    writeRaw(show ? "\033[?25h" : "\033[?25l");
}

void Terminal::clearScreen() {
    writeRaw("\033[2J\033[H");
}

void Terminal::setTitle(const std::string& title) {
    writeRaw("\033]0;" + title + "\007");
}

TermSize Terminal::getSize() const {
    return size_;
}

void Terminal::write(const std::string& data) {
    writeRaw(data);
}

void Terminal::writeRaw(const std::string& data) {
    ::write(STDOUT_FILENO, data.data(), data.size());
}

int Terminal::read(char* buf, size_t count) {
    return static_cast<int>(::read(STDIN_FILENO, buf, count));
}

void Terminal::detectSize() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        size_.rows = ws.ws_row;
        size_.cols = ws.ws_col;
    } else {
        size_.rows = 24;
        size_.cols = 80;
    }
}

void Terminal::handleSigwinch() {
    signal(SIGWINCH, sigwinchHandler);
}

void Terminal::sigwinchHandler(int) {
    if (instance_) {
        instance_->detectSize();
        if (instance_->resizeCallback_) {
            instance_->resizeCallback_(instance_->size_);
        }
    }
}

} // namespace tdesktop
