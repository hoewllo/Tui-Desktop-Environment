#include "config.h"
#include "helpers.h"
#include "logger.h"
#include <fstream>
#include <cstdlib>
#include <sstream>

namespace tdesktop {

ConfigManager::ConfigManager() {
    config_path_ = findConfigPath();
}

std::string ConfigManager::findConfigPath() const {
    const char* xdg = getenv("XDG_CONFIG_HOME");
    std::string dir;
    if (xdg) {
        dir = std::string(xdg);
    } else {
        const char* home = getenv("HOME");
        if (home) dir = std::string(home) + "/.config";
        else dir = "/tmp";
    }
    return dir + "/tdesktop/tdesktop.conf";
}

std::string ConfigManager::getConfigDir() const {
    size_t pos = config_path_.find_last_of('/');
    if (pos != std::string::npos) return config_path_.substr(0, pos);
    return "";
}

bool ConfigManager::load(const std::string& path) {
    std::string p = path.empty() ? config_path_ : path;
    std::ifstream f(p);
    if (!f.is_open()) {
        LOG_WARN("Config file not found: " + p);
        return false;
    }

    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    if (!parseJson(content)) {
        LOG_ERROR("Failed to parse config file");
        return false;
    }

    config_path_ = p;
    LOG_INFO("Config loaded from " + p);
    return true;
}

bool ConfigManager::save(const std::string& path) {
    std::string p = path.empty() ? config_path_ : path;

    // Ensure directory exists
    std::string dir = p.substr(0, p.find_last_of('/'));
    std::string mkdir = "mkdir -p " + dir;
    system(mkdir.c_str());

    std::ofstream f(p);
    if (!f.is_open()) {
        LOG_ERROR("Failed to write config: " + p);
        return false;
    }

    f << toJson();
    LOG_INFO("Config saved to " + p);
    return true;
}

bool ConfigManager::createDefault(const std::string& path) {
    config_ = Config{};
    return save(path);
}

bool ConfigManager::parseJson(const std::string& content) {
    // Simple JSON parsing
    std::string s = content;
    size_t pos = 0;

    auto skipWS = [&]() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
            pos++;
    };

    auto expect = [&](char c) -> bool {
        skipWS();
        if (pos < s.size() && s[pos] == c) { pos++; return true; }
        return false;
    };

    auto readString = [&]() -> std::string {
        skipWS();
        if (pos >= s.size() || s[pos] != '"') return "";
        pos++;
        std::string result;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\') { pos++; if (pos < s.size()) result += s[pos]; }
            else result += s[pos];
            pos++;
        }
        if (pos < s.size()) pos++;
        return result;
    };

    auto readValue = [&]() -> std::string {
        skipWS();
        if (pos >= s.size()) return "";
        if (s[pos] == '"') return readString();
        std::string val;
        while (pos < s.size() && s[pos] != ',' && s[pos] != '}' && s[pos] != ']' &&
               s[pos] != ' ' && s[pos] != '\t' && s[pos] != '\n' && s[pos] != '\r') {
            val += s[pos++];
        }
        return val;
    };

    if (!expect('{')) return false;

    while (pos < s.size() && s[pos] != '}') {
        skipWS();
        if (s[pos] == '}') break;

        std::string key = readString();
        if (!expect(':')) break;

        skipWS();
        if (s[pos] == '{') {
            // Nested object - skip for simplicity
            int depth = 1;
            pos++;
            while (pos < s.size() && depth > 0) {
                if (s[pos] == '{') depth++;
                else if (s[pos] == '}') depth--;
                pos++;
            }
        } else if (s[pos] == '[') {
            // Array
            pos++;
            std::vector<std::string> items;
            while (pos < s.size() && s[pos] != ']') {
                skipWS();
                if (s[pos] == ']') break;
                std::string val = readValue();
                if (!val.empty()) items.push_back(val);
                expect(',');
            }
            if (pos < s.size()) pos++;

            if (key == "startup_apps") {
                config_.startup_apps = items;
            }
        } else {
            std::string val = readValue();

            if (key == "theme") config_.theme = val;
            else if (key == "panel_height") config_.panel_height = std::stoi(val);
            else if (key == "workspaces") config_.workspaces = std::stoi(val);
            else if (key == "default_shell") config_.default_shell = val;
            else if (key == "history_size") config_.history_size = std::stoi(val);
            else if (key == "debug") config_.debug = (val == "true");
        }

        expect(',');
    }

    return true;
}

std::string ConfigManager::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "    \"theme\": \"" << config_.theme << "\",\n";
    json << "    \"panel_height\": " << config_.panel_height << ",\n";
    json << "    \"workspaces\": " << config_.workspaces << ",\n";
    json << "    \"default_shell\": \"" << config_.default_shell << "\",\n";
    json << "    \"debug\": " << (config_.debug ? "true" : "false") << ",\n";
    json << "    \"startup_apps\": [";
    for (size_t i = 0; i < config_.startup_apps.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << config_.startup_apps[i] << "\"";
    }
    json << "]\n";
    json << "}\n";
    return json.str();
}

} // namespace tdesktop
