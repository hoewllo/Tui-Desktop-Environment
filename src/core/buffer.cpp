#include "buffer.h"
#include <algorithm>
#include <cstring>
#include <sstream>

namespace tdesktop {

Rect Rect::intersect(const Rect& o) const {
    int x1 = std::max(x, o.x);
    int y1 = std::max(y, o.y);
    int x2 = std::min(x + w, o.x + o.w);
    int y2 = std::min(y + h, o.y + o.h);
    if (x1 < x2 && y1 < y2) return {x1, y1, x2 - x1, y2 - y1};
    return {0, 0, 0, 0};
}

Rect Rect::united(const Rect& o) const {
    int x1 = std::min(x, o.x);
    int y1 = std::min(y, o.y);
    int x2 = std::max(x + w, o.x + o.w);
    int y2 = std::max(y + h, o.y + o.h);
    return {x1, y1, x2 - x1, y2 - y1};
}

Buffer::Buffer(int cols, int rows)
    : cols_(cols), rows_(rows), cells_(static_cast<size_t>(cols) * rows) {
}

void Buffer::resize(int cols, int rows) {
    cols_ = cols;
    rows_ = rows;
    cells_.resize(static_cast<size_t>(cols) * rows);
    markAllDirty();
}

void Buffer::clear() {
    for (auto& cell : cells_) {
        cell = CharCell{};
    }
    markAllDirty();
}

void Buffer::clearRect(const Rect& rect) {
    for (int y = rect.y; y < rect.y + rect.h && y < rows_; ++y) {
        for (int x = rect.x; x < rect.x + rect.w && x < cols_; ++x) {
            if (x >= 0 && y >= 0) {
                cells_[index(x, y)] = CharCell{};
            }
        }
    }
    markDirty(rect.x, rect.y, rect.w, rect.h);
}

CharCell& Buffer::at(int x, int y) {
    return cells_[index(x, y)];
}

const CharCell& Buffer::at(int x, int y) const {
    return cells_[index(x, y)];
}

void Buffer::setChar(int x, int y, char32_t ch) {
    if (x < 0 || x >= cols_ || y < 0 || y >= rows_) return;
    auto& cell = cells_[index(x, y)];
    if (cell.ch != ch) {
        cell.ch = ch;
        cell.dirty = true;
        addDirtyRect({x, y, 1, 1});
    }
}

void Buffer::setFG(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= cols_ || y < 0 || y >= rows_) return;
    auto& cell = cells_[index(x, y)];
    if (cell.fg_r != r || cell.fg_g != g || cell.fg_b != b) {
        cell.fg_r = r; cell.fg_g = g; cell.fg_b = b;
        cell.use_rgb = true;
        cell.dirty = true;
        addDirtyRect({x, y, 1, 1});
    }
}

void Buffer::setBG(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= cols_ || y < 0 || y >= rows_) return;
    auto& cell = cells_[index(x, y)];
    if (cell.bg_r != r || cell.bg_g != g || cell.bg_b != b) {
        cell.bg_r = r; cell.bg_g = g; cell.bg_b = b;
        cell.use_rgb = true;
        cell.dirty = true;
        addDirtyRect({x, y, 1, 1});
    }
}

void Buffer::setStyle(int x, int y, bool bold, bool dim, bool italic,
                      bool underline, bool blink, bool reverse) {
    if (x < 0 || x >= cols_ || y < 0 || y >= rows_) return;
    auto& cell = cells_[index(x, y)];
    cell.bold = bold; cell.dim = dim; cell.italic = italic;
    cell.underline = underline; cell.blink = blink; cell.reverse = reverse;
    cell.dirty = true;
    addDirtyRect({x, y, 1, 1});
}

void Buffer::markDirty(int x, int y, int w, int h) {
    addDirtyRect({x, y, w, h});
}

void Buffer::markAllDirty() {
    dirtyRects_.clear();
    dirtyRects_.push_back({0, 0, cols_, rows_});
    for (auto& cell : cells_) cell.dirty = true;
}

void Buffer::markClean(int x, int y, int w, int h) {
    for (int row = y; row < y + h && row < rows_; ++row) {
        for (int col = x; col < x + w && col < cols_; ++col) {
            cells_[index(col, row)].dirty = false;
        }
    }
}

Rect Buffer::getDirtyBounds() const {
    if (dirtyRects_.empty()) return {0, 0, 0, 0};
    Rect b = dirtyRects_[0];
    for (size_t i = 1; i < dirtyRects_.size(); ++i) {
        b = b.united(dirtyRects_[i]);
    }
    return b;
}

std::vector<Rect> Buffer::getDirtyRects() const {
    return dirtyRects_;
}

void Buffer::clearDirty() {
    dirtyRects_.clear();
}

void Buffer::addDirtyRect(const Rect& r) {
    if (r.empty()) return;
    dirtyRects_.push_back(r);
    if (dirtyRects_.size() > 100) mergeDirtyRects();
}

void Buffer::mergeDirtyRects() {
    if (dirtyRects_.empty()) return;
    Rect merged = dirtyRects_[0];
    for (size_t i = 1; i < dirtyRects_.size(); ++i) {
        merged = merged.united(dirtyRects_[i]);
    }
    dirtyRects_.clear();
    dirtyRects_.push_back(merged);
}

std::string Buffer::render(const Buffer& prev) const {
    std::ostringstream out;
    auto rects = getDirtyRects();
    if (rects.empty()) return "";

    // Non-const self for merging (logically const for rendering)
    const_cast<Buffer*>(this)->mergeDirtyRects();
    rects = getDirtyRects();

    for (const auto& rect : rects) {
        for (int y = rect.y; y < rect.y + rect.h && y < rows_; ++y) {
            out << "\033[" << (y + 1) << ";" << (rect.x + 1) << "H";
            CharCell prev_style{};
            for (int x = rect.x; x < rect.x + rect.w && x < cols_; ++x) {
                const auto& cell = cells_[index(x, y)];
                std::string esc = escapeSequence(cell, prev_style);
                if (!esc.empty()) out << esc;
                if (cell.ch < 128) {
                    out << static_cast<char>(cell.ch);
                } else {
                    // Simple UTF-8 encoding for codepoints up to U+FFFF
                    char32_t cp = cell.ch;
                    if (cp < 0x80) {
                        out << static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        out << static_cast<char>(0xC0 | (cp >> 6));
                        out << static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out << static_cast<char>(0xE0 | (cp >> 12));
                        out << static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out << static_cast<char>(0x80 | (cp & 0x3F));
                    }
                }
                prev_style = cell;
            }
        }
    }
    return out.str();
}

std::string Buffer::escapeSequence(const CharCell& cur, const CharCell& prev) const {
    if (cur == prev) return "";
    std::ostringstream esc;
    esc << "\033[";
    bool first = true;

    auto addAttr = [&](int code) {
        if (!first) esc << ";";
        esc << code;
        first = false;
    };

    if (cur.bold != prev.bold) addAttr(cur.bold ? 1 : 22);
    if (cur.dim != prev.dim) addAttr(cur.dim ? 2 : 22);
    if (cur.italic != prev.italic) addAttr(cur.italic ? 3 : 23);
    if (cur.underline != prev.underline) addAttr(cur.underline ? 4 : 24);
    if (cur.blink != prev.blink) addAttr(cur.blink ? 5 : 25);
    if (cur.reverse != prev.reverse) addAttr(cur.reverse ? 7 : 27);

    if (cur.fg_r != prev.fg_r || cur.fg_g != prev.fg_g || cur.fg_b != prev.fg_b ||
        cur.fg_idx != prev.fg_idx || cur.use_rgb != prev.use_rgb) {
        if (cur.use_rgb) {
            addAttr(38);
            addAttr(2);
            addAttr(cur.fg_r);
            addAttr(cur.fg_g);
            addAttr(cur.fg_b);
        } else {
            addAttr(38);
            addAttr(5);
            addAttr(cur.fg_idx);
        }
    }

    if (cur.bg_r != prev.bg_r || cur.bg_g != prev.bg_g || cur.bg_b != prev.bg_b ||
        cur.bg_idx != prev.bg_idx || cur.use_rgb != prev.use_rgb) {
        if (cur.use_rgb) {
            addAttr(48);
            addAttr(2);
            addAttr(cur.bg_r);
            addAttr(cur.bg_g);
            addAttr(cur.bg_b);
        } else {
            addAttr(48);
            addAttr(5);
            addAttr(cur.bg_idx);
        }
    }

    if (first) return "";
    esc << "m";
    return esc.str();
}

} // namespace tdesktop
