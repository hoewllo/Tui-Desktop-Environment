#pragma once
#include <string>
#include <functional>
#include <termios.h>
#include <unistd.h>

namespace tdesktop {

struct TermSize {
    int rows = 0;
    int cols = 0;
};

class Terminal {
public:
    Terminal();
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    bool init();
    void restore();
    bool isInitialized() const { return initialized_; }

    void enableRawMode();
    void disableRawMode();
    void enterAlternateScreen();
    void exitAlternateScreen();
    void showCursor(bool show);
    void clearScreen();
    void setTitle(const std::string& title);

    TermSize getSize() const;
    int getRows() const { return size_.rows; }
    int getCols() const { return size_.cols; }

    void write(const std::string& data);
    void writeRaw(const std::string& data);
    int read(char* buf, size_t count);

    void onResize(std::function<void(TermSize)> cb) { resizeCallback_ = std::move(cb); }

private:
    bool initialized_ = false;
    TermSize size_{};
    termios original_termios_{};
    int saved_stdout_;
    std::function<void(TermSize)> resizeCallback_;

    void detectSize();
    void handleSigwinch();
    static void sigwinchHandler(int sig);
    static Terminal* instance_;
};

} // namespace tdesktop
