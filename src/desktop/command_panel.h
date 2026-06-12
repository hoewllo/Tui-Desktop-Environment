#pragma once
#include "../core/buffer.h"
#include "../core/renderer.h"
#include "../widgets/textbox.h"
#include <string>
#include <vector>
#include <deque>
#include <functional>

namespace tdesktop {

class CommandPanel {
public:
    CommandPanel();
    ~CommandPanel();

    CommandPanel(const CommandPanel&) = delete;
    CommandPanel& operator=(const CommandPanel&) = delete;

    void init();
    void toggle();
    void open();
    void close();
    bool isOpen() const { return open_; }

    void draw(Renderer& r, int cols, int rows);
    bool handleKey(const KeyEvent& ev);

    void setHeight(int h) { height_ = h; }
    int getHeight() const { return height_; }

    void onCommand(std::function<void(const std::string&)> cb) { cmd_cb_ = std::move(cb); }

    void clearOutput();

private:
    bool open_ = false;
    int height_ = 10;
    int input_row_ = 0;
    int pid_ = -1;
    int master_fd_ = -1;

    std::string input_buffer_;
    int cursor_pos_ = 0;
    std::deque<std::string> history_;
    int history_pos_ = -1;
    std::vector<std::string> output_lines_;
    int scroll_offset_ = 0;

    std::function<void(const std::string&)> cmd_cb_;

    void executeCommand();
    void addToHistory(const std::string& cmd);
    void spawnShell();
    void killShell();
    void readOutput();
    void writeInput(const std::string& data);
    bool hasShell() const { return pid_ > 0; }
};

} // namespace tdesktop
