#pragma once
#include "buffer.h"
#include "color.h"
#include <string>
#include <vector>
#include <chrono>

namespace tdesktop {

class Renderer {
public:
    Renderer(Buffer& front, Buffer& back, ColorManager& colors);
    ~Renderer() = default;

    void present();
    void clear();

    void drawChar(int x, int y, char32_t ch);
    void drawText(int x, int y, const std::string& text);
    void drawText(int x, int y, const std::u32string& text);
    void drawRect(const Rect& rect, bool fill = false);
    void drawRectOutline(const Rect& rect);

    void setFG(uint8_t r, uint8_t g, uint8_t b);
    void setBG(uint8_t r, uint8_t g, uint8_t b);
    void setFGIdx(uint8_t idx);
    void setBGIdx(uint8_t idx);
    void setRGB(bool use_rgb);
    void setStyle(bool bold = false, bool dim = false, bool italic = false,
                  bool underline = false, bool blink = false, bool reverse = false);

    int getCols() const { return back_.getCols(); }
    int getRows() const { return back_.getRows(); }

    double getFPS() const { return fps_; }
    void updateFPS();

private:
    Buffer& front_;
    Buffer& back_;
    ColorManager& colors_;
    double fps_ = 0.0;
    int frame_count_ = 0;
    double fps_accum_ = 0.0;
    std::chrono::steady_clock::time_point last_fps_time_;

    // Current drawing state
    bool cur_rgb_ = false;
    uint8_t cur_fg_r_ = 0, cur_fg_g_ = 0, cur_fg_b_ = 0;
    uint8_t cur_bg_r_ = 0, cur_bg_g_ = 0, cur_bg_b_ = 0;
    uint8_t cur_fg_idx_ = 7, cur_bg_idx_ = 0;
    bool cur_bold_ = false, cur_dim_ = false, cur_italic_ = false;
    bool cur_underline_ = false, cur_blink_ = false, cur_reverse_ = false;
};

} // namespace tdesktop
