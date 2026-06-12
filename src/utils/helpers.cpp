#include "helpers.h"
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <sys/wait.h>

namespace tdesktop {
namespace helpers {

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) result.push_back(item);
    }
    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    for (auto& c : result) c = static_cast<char>(std::tolower(c));
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    for (auto& c : result) c = static_cast<char>(std::toupper(c));
    return result;
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

std::string utf32ToUtf8(char32_t cp) {
    std::string result;
    if (cp < 0x80) {
        result += static_cast<char>(cp);
    } else if (cp < 0x800) {
        result += static_cast<char>(0xC0 | (cp >> 6));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        result += static_cast<char>(0xE0 | (cp >> 12));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (cp >> 18));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return result;
}

char32_t utf8ToUtf32(const char*& ptr) {
    unsigned char lead = static_cast<unsigned char>(*ptr);
    char32_t cp;
    if ((lead & 0x80) == 0) {
        cp = lead;
        ptr += 1;
    } else if ((lead & 0xE0) == 0xC0) {
        cp = (lead & 0x1F) << 6 | (static_cast<unsigned char>(*(ptr+1)) & 0x3F);
        ptr += 2;
    } else if ((lead & 0xF0) == 0xE0) {
        cp = (lead & 0x0F) << 12 | (static_cast<unsigned char>(*(ptr+1)) & 0x3F) << 6 | (static_cast<unsigned char>(*(ptr+2)) & 0x3F);
        ptr += 3;
    } else if ((lead & 0xF8) == 0xF0) {
        cp = (lead & 0x07) << 18 | (static_cast<unsigned char>(*(ptr+1)) & 0x3F) << 12 | (static_cast<unsigned char>(*(ptr+2)) & 0x3F) << 6 | (static_cast<unsigned char>(*(ptr+3)) & 0x3F);
        ptr += 4;
    } else {
        cp = lead;
        ptr += 1;
    }
    return cp;
}

std::u32string utf8ToUtf32(const std::string& str) {
    std::u32string result;
    const char* ptr = str.data();
    const char* end = ptr + str.size();
    while (ptr < end) {
        result += utf8ToUtf32(ptr);
    }
    return result;
}

std::string utf32ToUtf8(const std::u32string& str) {
    std::string result;
    for (char32_t cp : str) {
        result += utf32ToUtf8(cp);
    }
    return result;
}

int utf8CharLen(unsigned char lead) {
    if ((lead & 0x80) == 0) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}

int utf8SeqLength(const std::string& str, size_t pos) {
    if (pos >= str.size()) return 0;
    return utf8CharLen(static_cast<unsigned char>(str[pos]));
}

std::string exec(const std::string& cmd) {
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        result += buf;
    }
    pclose(pipe);
    return result;
}

} // namespace helpers

SignalHandler* SignalHandler::instance_ = nullptr;

SignalHandler& SignalHandler::instance() {
    static SignalHandler inst;
    instance_ = &inst;
    return inst;
}

void SignalHandler::setup() {
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, &orig_int_);
    sigaction(SIGTERM, &sa, &orig_term_);
}

void SignalHandler::restore() {
    sigaction(SIGINT, &orig_int_, nullptr);
    sigaction(SIGTERM, &orig_term_, nullptr);
}

void SignalHandler::onShutdown(std::function<void()> cb) {
    shutdown_cb_ = std::move(cb);
}

void SignalHandler::handleSignal(int sig) {
    if (instance_ && instance_->shutdown_cb_) {
        instance_->shutdown_cb_();
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

} // namespace tdesktop
