#pragma once
#include <cstdint>
#include <string>
#include <array>

namespace tdesktop {

struct RGBColor {
    uint8_t r = 0, g = 0, b = 0;

    bool operator==(const RGBColor& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const RGBColor& o) const { return !(*this == o); }
};

struct ColorPair {
    RGBColor fg;
    RGBColor bg;
    uint8_t fg_idx = 0;
    uint8_t bg_idx = 0;
    bool use_rgb = false;

    bool operator==(const ColorPair& o) const {
        if (use_rgb != o.use_rgb) return false;
        if (use_rgb) return fg == o.fg && bg == o.bg;
        return fg_idx == o.fg_idx && bg_idx == o.bg_idx;
    }
    bool operator!=(const ColorPair& o) const { return !(*this == o); }
};

class ColorManager {
public:
    ColorManager();

    std::string setForeground(int idx) const;
    std::string setBackground(int idx) const;
    std::string setForegroundRGB(uint8_t r, uint8_t g, uint8_t b) const;
    std::string setBackgroundRGB(uint8_t r, uint8_t g, uint8_t b) const;
    std::string reset() const;
    std::string bold() const;
    std::string dim() const;
    std::string italic() const;
    std::string underline() const;
    std::string blink() const;
    std::string reverse() const;

    bool has256Color() const { return color_depth_ >= 256; }
    bool has24bitColor() const { return color_depth_ >= 16777216; }
    int colorDepth() const { return color_depth_; }
    void setColorDepth(int depth);
    void detectColorDepth();

    static constexpr int ANSI_BLACK = 0;
    static constexpr int ANSI_RED = 1;
    static constexpr int ANSI_GREEN = 2;
    static constexpr int ANSI_YELLOW = 3;
    static constexpr int ANSI_BLUE = 4;
    static constexpr int ANSI_MAGENTA = 5;
    static constexpr int ANSI_CYAN = 6;
    static constexpr int ANSI_WHITE = 7;

    static std::array<RGBColor, 256> initXtermPalette();

private:
    int color_depth_ = 256;
    static std::array<RGBColor, 256> xterm_palette_;
};

} // namespace tdesktop
