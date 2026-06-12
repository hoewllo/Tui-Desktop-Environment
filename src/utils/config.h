#pragma once
#include <string>
#include <map>
#include <vector>

namespace tdesktop {

struct Config {
    std::string theme = "default";
    int panel_height = 10;
    int workspaces = 4;
    bool mouse_enabled = true;
    bool mouse_hardware_priority = true;
    std::string default_shell = "/bin/bash";
    int history_size = 500;
    bool debug = false;

    struct KeyBinding {
        std::string action;
        std::string key;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
    };
    std::vector<KeyBinding> keybindings;

    std::vector<std::string> startup_apps;
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;

    bool load(const std::string& path = "");
    bool save(const std::string& path = "");
    bool createDefault(const std::string& path);
    std::string findConfigPath() const;

    Config& getConfig() { return config_; }
    const Config& getConfig() const { return config_; }

    std::string getConfigDir() const;

private:
    Config config_;
    std::string config_path_;

    bool parseJson(const std::string& content);
    std::string toJson() const;
};

} // namespace tdesktop
