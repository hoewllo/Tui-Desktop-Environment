#include "launcher.h"
#include "../utils/helpers.h"
#include "../utils/logger.h"
#include <fstream>
#include <algorithm>
#include <cstdlib>

namespace tdesktop {

Launcher::Launcher() {
    scanApplications();
}

void Launcher::scanApplications() {
    apps_.clear();
    scanDirectory("/usr/share/applications");
    scanDirectory("/usr/local/share/applications");
    const char* home = getenv("HOME");
    if (home) {
        scanDirectory(std::string(home) + "/.local/share/applications");
    }
    std::sort(apps_.begin(), apps_.end(),
        [](const AppEntry& a, const AppEntry& b) { return a.name < b.name; });
    LOG_INFO("Found " + std::to_string(apps_.size()) + " applications");
}

void Launcher::scanDirectory(const std::string& path) {
    std::ifstream dir;
    // Simple approach: try to read common .desktop files
    std::string cmd = "ls " + path + "/*.desktop 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        line = helpers::trim(line);
        if (line.empty()) continue;

        std::ifstream f(line);
        if (!f.is_open()) continue;

        AppEntry app;
        std::string s;
        bool no_display = false;
        while (std::getline(f, s)) {
            s = helpers::trim(s);
            if (s.find("Name=") == 0 && app.name.empty())
                app.name = s.substr(5);
            else if (s.find("Exec=") == 0)
                app.exec = s.substr(5);
            else if (s.find("Comment=") == 0)
                app.comment = s.substr(8);
            else if (s.find("Icon=") == 0)
                app.icon = s.substr(5);
            else if (s.find("Categories=") == 0)
                app.categories = s.substr(11);
            else if (s.find("NoDisplay=true") == 0)
                no_display = true;
        }
        if (!app.name.empty() && !no_display) {
            // Clean up exec field
            size_t pos = app.exec.find("%");
            if (pos != std::string::npos) app.exec = app.exec.substr(0, pos);
            app.exec = helpers::trim(app.exec);
            apps_.push_back(app);
        }
    }
    pclose(pipe);
}

void Launcher::draw(Renderer& r, int cols, int rows) {
    if (!visible_) return;

    int box_w = 50;
    int box_h = 20;
    int box_x = (cols - box_w) / 2;
    int box_y = (rows - box_h) / 2;

    // Background
    r.setBGIdx(0);
    r.setFGIdx(7);
    r.drawRectOutline({box_x, box_y, box_w, box_h});

    // Title
    r.setFGIdx(15);
    r.setBGIdx(4);
    r.drawText(box_x + 2, box_y + 1, " Application Launcher ");

    // Search box
    r.setBGIdx(0);
    r.setFGIdx(7);
    r.drawText(box_x + 2, box_y + 3, "> ");
    r.drawText(box_x + 4, box_y + 3, search_text_);

    // Results
    int list_y = box_y + 5;
    int max_items = box_h - 7;
    int start = std::max(0, selected_idx_ - max_items + 1);

    filterApps();
    for (int i = start; i < start + max_items && i < static_cast<int>(filtered_.size()); ++i) {
        if (i == selected_idx_) {
            r.setFGIdx(0);
            r.setBGIdx(7);
        } else {
            r.setFGIdx(7);
            r.setBGIdx(0);
        }
        std::string entry = " " + filtered_[i].name;
        if (!filtered_[i].comment.empty()) {
            entry += " - " + filtered_[i].comment;
        }
        if (static_cast<int>(entry.size()) > box_w - 4) {
            entry = entry.substr(0, box_w - 7) + "...";
        }
        r.drawText(box_x + 2, list_y + (i - start), entry);
    }

    r.setFGIdx(7);
    r.setBGIdx(0);
    r.drawText(box_x + 2, box_y + box_h - 2, " Enter: launch  ESC: close ");
}

bool Launcher::handleKey(const KeyEvent& ev) {
    if (!visible_) return false;

    switch (ev.key) {
        case Key::Escape:
            visible_ = false;
            search_text_.clear();
            selected_idx_ = 0;
            return true;
        case Key::Enter:
            launchSelected();
            return true;
        case Key::Up:
            if (selected_idx_ > 0) selected_idx_--;
            return true;
        case Key::Down:
            if (selected_idx_ < static_cast<int>(filtered_.size()) - 1) selected_idx_++;
            return true;
        case Key::Backspace:
            if (!search_text_.empty()) {
                search_text_.pop_back();
                selected_idx_ = 0;
            }
            return true;
        default:
            if (ev.codepoint >= 32 && ev.codepoint < 127) {
                search_text_ += static_cast<char>(ev.codepoint);
                selected_idx_ = 0;
                return true;
            }
            break;
    }
    return false;
}

bool Launcher::handleClick(int x, int y) {
    if (!visible_) return false;
    // Basic click handling - close if clicked outside
    visible_ = false;
    search_text_.clear();
    selected_idx_ = 0;
    return true;
}

void Launcher::filterApps() {
    if (search_text_.empty()) {
        filtered_ = apps_;
    } else {
        filtered_.clear();
        std::string lower = helpers::toLower(search_text_);
        for (const auto& app : apps_) {
            if (helpers::toLower(app.name).find(lower) != std::string::npos) {
                filtered_.push_back(app);
            }
        }
    }
}

void Launcher::launchSelected() {
    if (filtered_.empty() || selected_idx_ >= static_cast<int>(filtered_.size())) return;
    std::string cmd = filtered_[selected_idx_].exec;
    LOG_INFO("Launching: " + cmd);
    if (!cmd.empty()) {
        pid_t pid = fork();
        if (pid == 0) {
            setsid();
            execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
            _exit(1);
        }
    }
    visible_ = false;
    search_text_.clear();
    selected_idx_ = 0;
}

} // namespace tdesktop
