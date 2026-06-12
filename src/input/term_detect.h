#pragma once
#include <string>

namespace tdesktop {

struct TermInfo {
    std::string type;
    std::string emulator;
    int color_depth = 256;
    bool alternate_screen = true;
    bool mouse_tracking = true;
    bool sgr_mouse = true;
    bool rgb_color = false;
    bool is_ssh = false;
    bool is_tmux = false;
    bool is_screen = false;
    bool is_vscode = false;

    int score() const;
    std::string summary() const;
};

class TermDetect {
public:
    TermDetect();
    ~TermDetect() = default;

    TermInfo detect();
    bool checkSSH();
    bool checkTmux();
    bool checkScreen();
    std::string detectEmulator();
    int detectColorDepth();

private:
    TermInfo info_;
};

} // namespace tdesktop
