#include "term_detect.h"
#include "../utils/helpers.h"
#include <cstdlib>

namespace tdesktop {

TermDetect::TermDetect() = default;

TermInfo TermDetect::detect() {
    info_ = TermInfo{};

    const char* term = getenv("TERM");
    info_.type = term ? term : "unknown";

    info_.emulator = detectEmulator();
    info_.is_ssh = checkSSH();
    info_.is_tmux = checkTmux();
    info_.is_screen = checkScreen();
    info_.color_depth = detectColorDepth();
    info_.rgb_color = info_.color_depth >= 16777216;

    // VS Code detection
    const char* vscode = getenv("VSCODE_INJECTION");
    if (vscode) info_.is_vscode = true;

    return info_;
}

bool TermDetect::checkSSH() {
    const char* ssh = getenv("SSH_CONNECTION");
    const char* ssh_client = getenv("SSH_CLIENT");
    return (ssh && ssh[0]) || (ssh_client && ssh_client[0]);
}

bool TermDetect::checkTmux() {
    const char* tmux = getenv("TMUX");
    return tmux != nullptr;
}

bool TermDetect::checkScreen() {
    const char* screen = getenv("STY");
    return screen != nullptr;
}

std::string TermDetect::detectEmulator() {
    const char* term_prog = getenv("TERM_PROGRAM");
    if (term_prog) return std::string(term_prog);

    std::string term = getenv("TERM") ? getenv("TERM") : "";
    if (term.find("xterm") != std::string::npos) return "xterm";
    if (term.find("alacritty") != std::string::npos) return "Alacritty";
    if (term.find("kitty") != std::string::npos) return "Kitty";
    if (term.find("gnome") != std::string::npos) return "GNOME Terminal";
    if (term.find("konsole") != std::string::npos) return "Konsole";

    return "unknown";
}

int TermDetect::detectColorDepth() {
    const char* colorterm = getenv("COLORTERM");
    if (colorterm) {
        std::string ct(colorterm);
        if (ct == "truecolor" || ct == "24bit") return 16777216;
    }

    std::string term = getenv("TERM") ? getenv("TERM") : "";
    if (term.find("truecolor") != std::string::npos) return 16777216;
    if (term.find("256") != std::string::npos) return 256;

    return 8;
}

int TermInfo::score() const {
    int s = 0;
    if (color_depth >= 16777216) s += 30;
    else if (color_depth >= 256) s += 20;
    else s += 10;

    if (alternate_screen) s += 15;
    if (mouse_tracking) s += 15;
    if (sgr_mouse) s += 10;
    if (!is_ssh) s += 10;
    if (!is_tmux && !is_screen) s += 10;

    return s;
}

std::string TermInfo::summary() const {
    std::string result = "Terminal: " + emulator + " (" + type + ")\n";
    result += "  Colors: " + std::to_string(color_depth) + "\n";
    result += "  SSH: " + std::string(is_ssh ? "yes" : "no") + "\n";
    result += "  tmux: " + std::string(is_tmux ? "yes" : "no") + "\n";
    result += "  RGB: " + std::string(rgb_color ? "yes" : "no") + "\n";
    result += "  Score: " + std::to_string(score()) + "/100";
    return result;
}

} // namespace tdesktop
