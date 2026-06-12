#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <signal.h>
#include <functional>

namespace tdesktop {

namespace helpers {

std::string trim(const std::string& str);
std::vector<std::string> split(const std::string& str, char delim);
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);
std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);
std::string replace(const std::string& str, const std::string& from, const std::string& to);

std::string utf32ToUtf8(char32_t cp);
char32_t utf8ToUtf32(const char*& ptr);
std::u32string utf8ToUtf32(const std::string& str);
std::string utf32ToUtf8(const std::u32string& str);

int utf8CharLen(unsigned char lead);
int utf8SeqLength(const std::string& str, size_t pos);

std::string exec(const std::string& cmd);

} // namespace helpers

class SignalHandler {
public:
    static SignalHandler& instance();
    void setup();
    void restore();
    void onShutdown(std::function<void()> cb);
    static void handleSignal(int sig);

private:
    SignalHandler() = default;
    struct sigaction orig_int_{}, orig_term_{};
    std::function<void()> shutdown_cb_;
    static SignalHandler* instance_;
};

} // namespace tdesktop
