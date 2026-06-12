#pragma once
#include <string>

namespace tdesktop {

class ShadowAuth {
public:
    ShadowAuth();
    ~ShadowAuth() = default;

    bool authenticate(const std::string& user, const std::string& pass);
    std::string getLastError() const { return last_error_; }
    bool isAvailable() const;

private:
    std::string last_error_;
};

} // namespace tdesktop
