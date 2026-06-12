#include "color.h"
#include <sstream>
#include <cstdlib>

namespace tdesktop {

std::array<RGBColor, 256> ColorManager::xterm_palette_ = initXtermPalette();

ColorManager::ColorManager() {
    detectColorDepth();
}

std::string ColorManager::setForeground(int idx) const {
    if (idx < 8) {
        return "\033[3" + std::to_string(idx) + "m";
    } else if (idx < 16) {
        return "\033[9" + std::to_string(idx - 8) + "m";
    }
    return "\033[38;5;" + std::to_string(idx) + "m";
}

std::string ColorManager::setBackground(int idx) const {
    if (idx < 8) {
        return "\033[4" + std::to_string(idx) + "m";
    } else if (idx < 16) {
        return "\033[10" + std::to_string(idx - 8) + "m";
    }
    return "\033[48;5;" + std::to_string(idx) + "m";
}

std::string ColorManager::setForegroundRGB(uint8_t r, uint8_t g, uint8_t b) const {
    if (!has24bitColor()) {
        // Fallback to closest 256-color
        int idx = 16 + 36 * (r * 5 / 256) + 6 * (g * 5 / 256) + (b * 5 / 256);
        return setForeground(idx);
    }
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string ColorManager::setBackgroundRGB(uint8_t r, uint8_t g, uint8_t b) const {
    if (!has24bitColor()) {
        int idx = 16 + 36 * (r * 5 / 256) + 6 * (g * 5 / 256) + (b * 5 / 256);
        return setBackground(idx);
    }
    return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

std::string ColorManager::reset() const {
    return "\033[0m";
}

std::string ColorManager::bold() const { return "\033[1m"; }
std::string ColorManager::dim() const { return "\033[2m"; }
std::string ColorManager::italic() const { return "\033[3m"; }
std::string ColorManager::underline() const { return "\033[4m"; }
std::string ColorManager::blink() const { return "\033[5m"; }
std::string ColorManager::reverse() const { return "\033[7m"; }

void ColorManager::setColorDepth(int depth) {
    color_depth_ = depth;
}

void ColorManager::detectColorDepth() {
    const char* term = getenv("TERM");
    if (!term) { color_depth_ = 8; return; }

    std::string t(term);
    if (t.find("truecolor") != std::string::npos || t.find("24bit") != std::string::npos) {
        color_depth_ = 16777216;
    } else if (t.find("256") != std::string::npos) {
        color_depth_ = 256;
    } else {
        color_depth_ = 8;
    }

    const char* colorterm = getenv("COLORTERM");
    if (colorterm) {
        std::string ct(colorterm);
        if (ct == "truecolor" || ct == "24bit") {
            color_depth_ = 16777216;
        }
    }
}

std::array<RGBColor, 256> ColorManager::initXtermPalette() {
    std::array<RGBColor, 256> palette{};

    // Standard 16 ANSI colors
    palette[0] = {0, 0, 0};
    palette[1] = {128, 0, 0};
    palette[2] = {0, 128, 0};
    palette[3] = {128, 128, 0};
    palette[4] = {0, 0, 128};
    palette[5] = {128, 0, 128};
    palette[6] = {0, 128, 128};
    palette[7] = {192, 192, 192};
    palette[8] = {128, 128, 128};
    palette[9] = {255, 0, 0};
    palette[10] = {0, 255, 0};
    palette[11] = {255, 255, 0};
    palette[12] = {0, 0, 255};
    palette[13] = {255, 0, 255};
    palette[14] = {0, 255, 255};
    palette[15] = {255, 255, 255};

    // 216 color cube (16-231)
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                int idx = 16 + r * 36 + g * 6 + b;
                palette[idx] = {
                    static_cast<uint8_t>(r ? r * 40 + 55 : 0),
                    static_cast<uint8_t>(g ? g * 40 + 55 : 0),
                    static_cast<uint8_t>(b ? b * 40 + 55 : 0)
                };
            }
        }
    }

    // Grayscale (232-255)
    for (int i = 0; i < 24; ++i) {
        int val = i * 10 + 8;
        palette[232 + i] = {
            static_cast<uint8_t>(val),
            static_cast<uint8_t>(val),
            static_cast<uint8_t>(val)
        };
    }

    return palette;
}

} // namespace tdesktop
