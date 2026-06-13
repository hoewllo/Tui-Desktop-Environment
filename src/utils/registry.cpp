#include "registry.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cstdlib>

namespace tdesktop {

Registry& Registry::instance() {
    static Registry reg;
    return reg;
}

Registry::~Registry() {
    save();
}

static std::string rootToString(RegRoot root) {
    switch (root) {
        case RegRoot::ClassesRoot:  return "HKEY_CLASSES_ROOT";
        case RegRoot::CurrentUser:  return "HKEY_CURRENT_USER";
        case RegRoot::LocalMachine: return "HKEY_LOCAL_MACHINE";
        case RegRoot::Users:        return "HKEY_USERS";
        case RegRoot::CurrentConfig:return "HKEY_CURRENT_CONFIG";
    }
    return "";
}

std::string Registry::rootName(RegRoot root) const {
    return rootToString(root);
}

std::string Registry::formatKey(RegRoot root, const std::string& path) const {
    std::string r = rootToString(root);
    if (path.empty()) return r;
    if (path[0] == '\\') return r + path;
    return r + "\\" + path;
}

static std::vector<std::string> splitKey(const std::string& key) {
    std::vector<std::string> parts;
    std::stringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '\\')) {
        if (!part.empty()) parts.push_back(part);
    }
    return parts;
}

Registry::Node* Registry::getNode(const std::string& key, bool create) {
    auto parts = splitKey(key);
    if (parts.empty()) {
        if (!root_ && create) root_ = std::make_unique<Node>();
        return root_.get();
    }

    if (!root_) {
        if (create) root_ = std::make_unique<Node>();
        else return nullptr;
    }

    Node* node = root_.get();
    for (const auto& part : parts) {
        auto it = node->children.find(part);
        if (it == node->children.end()) {
            if (!create) return nullptr;
            auto new_node = std::make_unique<Node>();
            node->children[part] = std::move(new_node);
            it = node->children.find(part);
        }
        node = it->second.get();
    }
    return node;
}

const Registry::Node* Registry::getNode(const std::string& key) const {
    return const_cast<Registry*>(this)->getNode(key, false);
}

void Registry::load(const std::string& path) {
    if (path.empty()) {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";
        file_path_ = std::string(home) + "/.config/tdesktop/registry.json";
    } else {
        file_path_ = path;
    }

    std::ifstream f(file_path_);
    if (!f.is_open()) {
        LOG_INFO("Registry file not found, using defaults");
        return;
    }

    root_ = std::make_unique<Node>();
    std::string line;
    Node* current = root_.get();
    std::string current_key;
    (void)current_key;

    // Simple JSON parser for registry format
    // Format: {"key": {"subkey": {"name": "value"}}}
    // Using the existing ConfigManager parser approach
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // Very simple parser - expects { "key\\path": { "valuename": "stringval", ... } }
    size_t pos = 0;
    auto skipSpace = [&]() {
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\n' ||
               content[pos] == '\r' || content[pos] == '\t')) pos++;
    };

    if (pos >= content.size() || content[pos] != '{') {
        LOG_WARN("Invalid registry file format");
        return;
    }
    pos++; // skip {

    while (pos < content.size()) {
        skipSpace();
        if (pos >= content.size() || content[pos] == '}') break;

        // Read key string
        if (content[pos] != '"') { pos++; continue; }
        pos++;
        std::string key_str;
        while (pos < content.size() && content[pos] != '"') {
            if (content[pos] == '\\' && pos + 1 < content.size()) {
                key_str += content[pos + 1];
                pos += 2;
            } else {
                key_str += content[pos++];
            }
        }
        if (pos < content.size()) pos++; // skip closing "

        skipSpace();
        if (pos >= content.size() || content[pos] != ':') { pos++; continue; }
        pos++; // skip :
        skipSpace();
        if (pos >= content.size() || content[pos] != '{') { pos++; continue; }
        pos++; // skip {

        Node* key_node = getNode(key_str, true);
        if (!key_node) { pos++; continue; }

        while (pos < content.size()) {
            skipSpace();
            if (pos >= content.size() || content[pos] == '}') break;

            if (content[pos] != '"') { pos++; continue; }
            pos++;
            std::string val_name;
            while (pos < content.size() && content[pos] != '"') {
                if (content[pos] == '\\' && pos + 1 < content.size()) {
                    val_name += content[pos + 1];
                    pos += 2;
                } else {
                    val_name += content[pos++];
                }
            }
            if (pos < content.size()) pos++; // skip "
            skipSpace();
            if (pos >= content.size() || content[pos] != ':') { pos++; continue; }
            pos++; // skip :
            skipSpace();

            if (pos >= content.size()) break;

            if (content[pos] == '"') {
                // String value
                pos++;
                std::string sval;
                while (pos < content.size() && content[pos] != '"') {
                    if (content[pos] == '\\' && pos + 1 < content.size()) {
                        sval += content[pos + 1];
                        pos += 2;
                    } else {
                        sval += content[pos++];
                    }
                }
                if (pos < content.size()) pos++;
                key_node->values[val_name] = sval;
            } else if (content[pos] == '-' || (content[pos] >= '0' && content[pos] <= '9')) {
                // Number
                std::string num;
                if (content[pos] == '-') { num += '-'; pos++; }
                while (pos < content.size() && content[pos] >= '0' && content[pos] <= '9') {
                    num += content[pos++];
                }
                key_node->values[val_name] = std::stoi(num);
            } else {
                // Skip unknown
                while (pos < content.size() && content[pos] != ',' && content[pos] != '}') pos++;
            }

            skipSpace();
            if (pos < content.size() && content[pos] == ',') pos++;
        }

        if (pos < content.size() && content[pos] == '}') pos++; // skip }
        skipSpace();
        if (pos < content.size() && content[pos] == ',') pos++;
    }

    LOG_INFO("Registry loaded from " + file_path_);
}

void Registry::save() {
    if (file_path_.empty() || !root_) return;

    // Create directory if needed
    size_t slash = file_path_.rfind('/');
    if (slash != std::string::npos) {
        std::string dir = file_path_.substr(0, slash);
        std::string mkdir = "mkdir -p " + dir;
        system(mkdir.c_str());
    }

    std::ofstream f(file_path_);
    if (!f.is_open()) {
        LOG_ERROR("Failed to save registry to " + file_path_);
        return;
    }

    f << "{\n";

    // Collect all keys recursively
    std::vector<std::pair<std::string, Node*>> keys;
    std::function<void(const std::string&, Node*)> collect;
    collect = [&](const std::string& prefix, Node* node) {
        for (auto& [name, child] : node->children) {
            std::string key_path = prefix.empty() ? name : prefix + "\\" + name;
            keys.push_back({key_path, child.get()});
            collect(key_path, child.get());
        }
    };
    collect("", root_.get());

    bool first = true;
    for (auto& [key_path, node] : keys) {
        if (!node->values.empty()) {
            if (!first) f << ",\n";
            first = false;
            f << "  \"" << key_path << "\": {\n";
            bool vfirst = true;
            for (auto& [vname, vval] : node->values) {
                if (!vfirst) f << ",\n";
                vfirst = false;
                if (std::holds_alternative<std::string>(vval)) {
                    f << "    \"" << vname << "\": \"" << std::get<std::string>(vval) << "\"";
                } else if (std::holds_alternative<int>(vval)) {
                    f << "    \"" << vname << "\": " << std::get<int>(vval);
                }
            }
            f << "\n  }";
        }
    }

    f << "\n}\n";
    LOG_INFO("Registry saved to " + file_path_);
}

bool Registry::keyExists(const std::string& key) const {
    return getNode(key) != nullptr;
}

bool Registry::valueExists(const std::string& key, const std::string& name) const {
    auto* node = getNode(key);
    return node && node->values.find(name) != node->values.end();
}

void Registry::deleteKey(const std::string& key) {
    auto parts = splitKey(key);
    if (parts.empty()) return;

    if (parts.size() == 1) {
        if (root_) root_->children.erase(parts[0]);
        return;
    }

    std::string parent_key;
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!parent_key.empty()) parent_key += "\\";
        parent_key += parts[i];
    }

    auto* parent = getNode(parent_key);
    if (parent) {
        parent->children.erase(parts.back());
    }
}

void Registry::deleteValue(const std::string& key, const std::string& name) {
    auto* node = getNode(key);
    if (node) {
        node->values.erase(name);
    }
}

std::vector<std::string> Registry::enumKeys(const std::string& key) const {
    std::vector<std::string> result;
    auto* node = getNode(key);
    if (node) {
        for (auto& [name, _] : node->children) {
            result.push_back(name);
        }
    }
    return result;
}

std::vector<std::string> Registry::enumValues(const std::string& key) const {
    std::vector<std::string> result;
    auto* node = getNode(key);
    if (node) {
        for (auto& [name, _] : node->values) {
            result.push_back(name);
        }
    }
    return result;
}

std::string Registry::getString(const std::string& key, const std::string& name, const std::string& def) const {
    auto* node = getNode(key);
    if (!node) return def;
    auto it = node->values.find(name);
    if (it == node->values.end()) return def;
    if (!std::holds_alternative<std::string>(it->second)) return def;
    return std::get<std::string>(it->second);
}

int Registry::getDword(const std::string& key, const std::string& name, int def) const {
    auto* node = getNode(key);
    if (!node) return def;
    auto it = node->values.find(name);
    if (it == node->values.end()) return def;
    if (!std::holds_alternative<int>(it->second)) return def;
    return std::get<int>(it->second);
}

std::vector<uint8_t> Registry::getBinary(const std::string& key, const std::string& name) const {
    auto* node = getNode(key);
    if (!node) return {};
    auto it = node->values.find(name);
    if (it == node->values.end()) return {};
    if (!std::holds_alternative<std::vector<uint8_t>>(it->second)) return {};
    return std::get<std::vector<uint8_t>>(it->second);
}

void Registry::setString(const std::string& key, const std::string& name, const std::string& val) {
    auto* node = getNode(key, true);
    node->values[name] = val;
}

void Registry::setDword(const std::string& key, const std::string& name, int val) {
    auto* node = getNode(key, true);
    node->values[name] = val;
}

void Registry::setBinary(const std::string& key, const std::string& name, const std::vector<uint8_t>& val) {
    auto* node = getNode(key, true);
    node->values[name] = val;
}

} // namespace tdesktop
