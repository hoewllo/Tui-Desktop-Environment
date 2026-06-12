#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace tdesktop {

struct CharCell {
    char32_t ch = L' ';
    uint8_t fg_r = 0, fg_g = 0, fg_b = 0;
    uint8_t bg_r = 0, bg_g = 0, bg_b = 0;
    uint8_t fg_idx = 0;
    uint8_t bg_idx = 0;
    bool use_rgb = false;
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
    bool blink = false;
    bool reverse = false;
    bool dirty = true;

    bool operator==(const CharCell& o) const {
        return ch == o.ch &&
               fg_r == o.fg_r && fg_g == o.fg_g && fg_b == o.fg_b &&
               bg_r == o.bg_r && bg_g == o.bg_g && bg_b == o.bg_b &&
               fg_idx == o.fg_idx && bg_idx == o.bg_idx &&
               use_rgb == o.use_rgb &&
               bold == o.bold && dim == o.dim &&
               italic == o.italic && underline == o.underline &&
               blink == o.blink && reverse == o.reverse;
    }
    bool operator!=(const CharCell& o) const { return !(*this == o); }
};

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;

    bool empty() const { return w <= 0 || h <= 0; }
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    Rect intersect(const Rect& o) const;
    Rect united(const Rect& o) const;
};

class Buffer {
public:
    Buffer(int cols, int rows);
    ~Buffer() = default;

    void resize(int cols, int rows);
    void clear();
    void clearRect(const Rect& rect);

    CharCell& at(int x, int y);
    const CharCell& at(int x, int y) const;

    void setChar(int x, int y, char32_t ch);
    void setFG(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void setBG(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void setStyle(int x, int y, bool bold, bool dim, bool italic,
                  bool underline, bool blink, bool reverse);

    void markDirty(int x, int y, int w, int h);
    void markAllDirty();
    void markClean(int x, int y, int w, int h);

    Rect getDirtyBounds() const;
    std::vector<Rect> getDirtyRects() const;
    void clearDirty();

    int getCols() const { return cols_; }
    int getRows() const { return rows_; }
    size_t index(int x, int y) const { return static_cast<size_t>(y) * cols_ + x; }

    std::string render(const Buffer& prev) const;

private:
    int cols_, rows_;
    std::vector<CharCell> cells_;
    std::vector<Rect> dirtyRects_;

    void mergeDirtyRects();
    void addDirtyRect(const Rect& r);

    std::string escapeSequence(const CharCell& cur, const CharCell& prev) const;
};

} // namespace tdesktop
