#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

namespace tdesktop {

enum class RegRoot {
    ClassesRoot,
    CurrentUser,
    LocalMachine,
    Users,
    CurrentConfig
};

using RegValue = std::variant<std::string, int, std::vector<uint8_t>>;

struct RegEntry {
    std::string name;
    RegValue value;
};

class Registry {
public:
    static Registry& instance();

    void load(const std::string& path = "");
    void save();

    // Read
    std::string getString(const std::string& key, const std::string& name, const std::string& def = "") const;
    int getDword(const std::string& key, const std::string& name, int def = 0) const;
    std::vector<uint8_t> getBinary(const std::string& key, const std::string& name) const;

    // Write
    void setString(const std::string& key, const std::string& name, const std::string& val);
    void setDword(const std::string& key, const std::string& name, int val);
    void setBinary(const std::string& key, const std::string& name, const std::vector<uint8_t>& val);

    // Key management
    bool keyExists(const std::string& key) const;
    bool valueExists(const std::string& key, const std::string& name) const;
    void deleteKey(const std::string& key);
    void deleteValue(const std::string& key, const std::string& name);
    std::vector<std::string> enumKeys(const std::string& key) const;
    std::vector<std::string> enumValues(const std::string& key) const;

    std::string formatKey(RegRoot root, const std::string& path) const;
    std::string rootName(RegRoot root) const;

private:
    Registry() = default;
    ~Registry();
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    struct Node {
        std::map<std::string, RegValue> values;
        std::map<std::string, std::unique_ptr<Node>> children;
    };

    Node* getNode(const std::string& key, bool create = false);
    const Node* getNode(const std::string& key) const;

    std::unique_ptr<Node> root_;
    std::string file_path_;
};

} // namespace tdesktop
