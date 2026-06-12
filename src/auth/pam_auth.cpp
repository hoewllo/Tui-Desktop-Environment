#include "pam_auth.h"
#include "../utils/logger.h"
#include <security/pam_appl.h>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

namespace tdesktop {

struct PamData {
    const char* password;
};

static int pamConv(int num_msg, const struct pam_message** msg,
                   struct pam_response** resp, void* appdata_ptr) {
    if (num_msg <= 0 || !resp) return PAM_CONV_ERR;
    auto* data = static_cast<PamData*>(appdata_ptr);
    *resp = static_cast<struct pam_response*>(calloc(num_msg, sizeof(struct pam_response)));
    if (!*resp) return PAM_BUF_ERR;

    for (int i = 0; i < num_msg; ++i) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF ||
            msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            if (data->password) {
                (*resp)[i].resp = strdup(data->password);
            } else {
                (*resp)[i].resp = strdup("");
            }
        }
    }
    return PAM_SUCCESS;
}

PAMAuth::PAMAuth() = default;

PAMAuth::~PAMAuth() = default;

bool PAMAuth::init() {
    // Check if running as root - PAM works regardless
    return true;
}

bool PAMAuth::authenticate(const std::string& user, const std::string& pass) {
    attempts_++;

    const char* service_name = "tdesktop";
    pam_handle_t* pamh = nullptr;
    PamData data{pass.c_str()};

    struct pam_conv conv = {pamConv, &data};

    int ret = pam_start(service_name, user.c_str(), &conv, &pamh);
    if (ret != PAM_SUCCESS) {
        last_error_ = pam_strerror(pamh, ret);
        LOG_ERROR("PAM start failed: " + last_error_);
        return false;
    }

    ret = pam_authenticate(pamh, 0);
    if (ret != PAM_SUCCESS) {
        last_error_ = pam_strerror(pamh, ret);
        LOG_WARN("PAM auth failed for user '" + user + "': " + last_error_);
        pam_end(pamh, ret);
        return false;
    }

    ret = pam_acct_mgmt(pamh, 0);
    if (ret != PAM_SUCCESS) {
        last_error_ = pam_strerror(pamh, ret);
        LOG_WARN("PAM account check failed: " + last_error_);
        pam_end(pamh, ret);
        return false;
    }

    pam_end(pamh, PAM_SUCCESS);
    authenticated_ = true;
    getUserInfo(user);
    LOG_INFO("User '" + user + "' authenticated successfully");
    return true;
}

void PAMAuth::getUserInfo(const std::string& user) {
    struct passwd* pw = getpwnam(user.c_str());
    if (pw) {
        display_name_ = pw->pw_gecos ? pw->pw_gecos : user;
        home_dir_ = pw->pw_dir ? pw->pw_dir : "";
        shell_ = pw->pw_shell ? pw->pw_shell : "";
    }
}

} // namespace tdesktop
