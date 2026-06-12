#include "shadow_auth.h"
#include "../utils/logger.h"
#include <shadow.h>
#include <cstring>
#include <unistd.h>

#ifdef __linux__
#include <crypt.h>
#endif

namespace tdesktop {

ShadowAuth::ShadowAuth() = default;

bool ShadowAuth::authenticate(const std::string& user, const std::string& pass) {
    struct spwd* sp = getspnam(user.c_str());
    if (!sp) {
        last_error_ = "User not found in /etc/shadow";
        LOG_WARN(last_error_);
        return false;
    }

    if (!sp->sp_pwdp || sp->sp_pwdp[0] == '\0') {
        last_error_ = "No password set for user";
        LOG_WARN(last_error_);
        return false;
    }

    // Check if account is locked
    if (sp->sp_pwdp[0] == '!' || sp->sp_pwdp[0] == '*') {
        last_error_ = "Account is locked";
        LOG_WARN(last_error_);
        return false;
    }

    const char* encrypted = crypt(pass.c_str(), sp->sp_pwdp);
    if (!encrypted) {
        last_error_ = "crypt() failed";
        LOG_ERROR(last_error_);
        return false;
    }

    if (strcmp(encrypted, sp->sp_pwdp) == 0) {
        LOG_INFO("Shadow auth successful for user '" + user + "'");
        return true;
    }

    last_error_ = "Invalid password";
    LOG_WARN("Shadow auth failed for user '" + user + "'");
    return false;
}

bool ShadowAuth::isAvailable() const {
    return getspnam("root") != nullptr || access("/etc/shadow", R_OK) == 0;
}

} // namespace tdesktop
