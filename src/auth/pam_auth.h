#pragma once
#include <string>
#include <functional>

namespace tdesktop {

class PAMAuth {
public:
    PAMAuth();
    ~PAMAuth();

    PAMAuth(const PAMAuth&) = delete;
    PAMAuth& operator=(const PAMAuth&) = delete;

    bool init();
    bool authenticate(const std::string& user, const std::string& pass);
    bool isAuthenticated() const { return authenticated_; }
    std::string getLastError() const { return last_error_; }
    int getAttempts() const { return attempts_; }
    int getMaxAttempts() const { return 3; }

    std::string getDisplayName() const { return display_name_; }
    std::string getHomeDir() const { return home_dir_; }
    std::string getShell() const { return shell_; }

private:
    bool authenticated_ = false;
    std::string last_error_;
    std::string display_name_;
    std::string home_dir_;
    std::string shell_;
    int attempts_ = 0;

    void getUserInfo(const std::string& user);
};

} // namespace tdesktop
