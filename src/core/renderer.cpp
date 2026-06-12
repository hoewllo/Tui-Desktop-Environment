#include "renderer.h"
#include <chrono>
#include <unistd.h>

namespace tdesktop {

Renderer::Renderer(Buffer& front, Buffer& back, ColorManager& colors)
    : front_(front), back_(back), colors_(colors) {
    last_fps_time_ = std::chrono::steady_clock::now();
}

void Renderer::present() {
    std::string output = back_.render(front_);
    if (!output.empty()) {
        ::write(STDOUT_FILENO, output.data(), output.size());
    }
    front_ = back_;
    back_.clearDirty();
    frame_count_++;
    updateFPS();
}

void Renderer::clear() {
    back_.clear();
    ::write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

void Renderer::drawChar(int x, int y, char32_t ch) {
    back_.setChar(x, y, ch);
    if (cur_rgb_) {
        back_.setFG(x, y, cur_fg_r_, cur_fg_g_, cur_fg_b_);
        back_.setBG(x, y, cur_bg_r_, cur_bg_g_, cur_bg_b_);
    } else {
        auto& cell = back_.at(x, y);
        cell.fg_idx = cur_fg_idx_;
        cell.bg_idx = cur_bg_idx_;
        cell.use_rgb = false;
        cell.dirty = true;
    }
    back_.setStyle(x, y, cur_bold_, cur_dim_, cur_italic_,
                   cur_underline_, cur_blink_, cur_reverse_);
}

void Renderer::drawText(int x, int y, const std::string& text) {
    int cx = x;
    for (size_t i = 0; i < text.size();) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        char32_t cp;
        int len;
        if ((c & 0x80) == 0) { cp = c; len = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = (c & 0x1F) << 6 | (text[i+1] & 0x3F); len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = (c & 0x0F) << 12 | (text[i+1] & 0x3F) << 6 | (text[i+2] & 0x3F); len = 3; }
        else if ((c & 0xF8) == 0xF0) { cp = (c & 0x07) << 18 | (text[i+1] & 0x3F) << 12 | (text[i+2] & 0x3F) << 6 | (text[i+3] & 0x3F); len = 4; }
        else { cp = c; len = 1; }
        drawChar(cx++, y, cp);
        i += len;
    }
}

void Renderer::drawText(int x, int y, const std::u32string& text) {
    for (size_t i = 0; i < text.size(); ++i) {
        drawChar(x + static_cast<int>(i), y, text[i]);
    }
}

void Renderer::drawRect(const Rect& rect, bool fill) {
    for (int row = rect.y; row < rect.y + rect.h && row < getRows(); ++row) {
        for (int col = rect.x; col < rect.x + rect.w && col < getCols(); ++col) {
            if (fill) {
                drawChar(col, row, L' ');
            } else if (row == rect.y || row == rect.y + rect.h - 1 ||
                       col == rect.x || col == rect.x + rect.w - 1) {
                drawChar(col, row, L' ');
            }
        }
    }
}

void Renderer::drawRectOutline(const Rect& rect) {
    // Corners
    drawChar(rect.x, rect.y, L'┌');
    drawChar(rect.x + rect.w - 1, rect.y, L'┐');
    drawChar(rect.x, rect.y + rect.h - 1, L'└');
    drawChar(rect.x + rect.w - 1, rect.y + rect.h - 1, L'┘');

    // Horizontal lines
    for (int x = rect.x + 1; x < rect.x + rect.w - 1; ++x) {
        drawChar(x, rect.y, L'─');
        drawChar(x, rect.y + rect.h - 1, L'─');
    }

    // Vertical lines
    for (int y = rect.y + 1; y < rect.y + rect.h - 1; ++y) {
        drawChar(rect.x, y, L'│');
        drawChar(rect.x + rect.w - 1, y, L'│');
    }
}

void Renderer::setFG(uint8_t r, uint8_t g, uint8_t b) {
    cur_rgb_ = true;
    cur_fg_r_ = r; cur_fg_g_ = g; cur_fg_b_ = b;
}

void Renderer::setBG(uint8_t r, uint8_t g, uint8_t b) {
    cur_rgb_ = true;
    cur_bg_r_ = r; cur_bg_g_ = g; cur_bg_b_ = b;
}

void Renderer::setFGIdx(uint8_t idx) {
    cur_rgb_ = false;
    cur_fg_idx_ = idx;
}

void Renderer::setBGIdx(uint8_t idx) {
    cur_rgb_ = false;
    cur_bg_idx_ = idx;
}

void Renderer::setRGB(bool use_rgb) {
    cur_rgb_ = use_rgb;
}

void Renderer::setStyle(bool bold, bool dim, bool italic,
                        bool underline, bool blink, bool reverse) {
    cur_bold_ = bold; cur_dim_ = dim; cur_italic_ = italic;
    cur_underline_ = underline; cur_blink_ = blink; cur_reverse_ = reverse;
}

void Renderer::updateFPS() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_time_).count();
    if (elapsed >= 1000) {
        fps_ = frame_count_ * 1000.0 / elapsed;
        frame_count_ = 0;
        last_fps_time_ = now;
    }
}

} // namespace tdesktop
