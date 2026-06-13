#include "core/terminal.h"
#include "core/event.h"
#include "core/color.h"
#include "core/buffer.h"
#include "core/renderer.h"
#include "desktop/desktop.h"
#include "auth/login.h"
#include "auth/pam_auth.h"
#include "auth/shadow_auth.h"
#include "notification/notify.h"
#include "notification/dbus_listener.h"
#include "utils/logger.h"
#include "utils/config.h"
#include "input/term_detect.h"
#include "utils/helpers.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <pty.h>
#include <sys/wait.h>

using namespace tdesktop;

struct Options {
    bool debug = false;
    bool no_mouse = false;
    bool force_mouse = false;
    bool check_env = false;
    bool reset_config = false;
    bool show_help = false;
    bool auto_login = false;
    std::string auto_user;
    bool cli_user = false;
    std::string cli_password;
    std::string config_path;
    std::string theme;
};

static Options parseArgs(int argc, char* argv[]) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") opts.show_help = true;
        else if (arg == "--version" || arg == "-v") {
            std::cout << "TUI Desktop Environment v1.0.0" << std::endl;
            exit(0);
        }
        else if (arg == "--debug") opts.debug = true;
        else if (arg == "--no-mouse") opts.no_mouse = true;
        else if (arg == "--force-mouse") opts.force_mouse = true;
        else if (arg == "--check-env") opts.check_env = true;
        else if (arg == "--reset-config") opts.reset_config = true;
        else if (arg.find("--auto-login=") == 0) {
            opts.auto_login = true;
            opts.auto_user = arg.substr(13);
        }
        else if (arg == "--auto-login" && i + 1 < argc) {
            opts.auto_login = true;
            opts.auto_user = argv[++i];
        }
        else if (arg.find("--user=") == 0) {
            opts.cli_user = true;
            opts.auto_user = arg.substr(7);
        }
        else if (arg == "--user" && i + 1 < argc) {
            opts.cli_user = true;
            opts.auto_user = argv[++i];
        }
        else if (arg.find("--password=") == 0) {
            opts.cli_password = arg.substr(11);
        }
        else if (arg == "--password" && i + 1 < argc) {
            opts.cli_password = argv[++i];
        }
        else if (arg.find("--config=") == 0) opts.config_path = arg.substr(9);
        else if (arg == "--config" && i + 1 < argc) opts.config_path = argv[++i];
        else if (arg.find("--theme=") == 0) opts.theme = arg.substr(8);
        else if (arg == "--theme" && i + 1 < argc) opts.theme = argv[++i];
    }
    return opts;
}

static void printHelp() {
    const char* help = R"(
TUI Desktop Environment v1.0.0
================================

Usage:
  tdesktop [options]

Options:
  --help, -h          Show this help message
  --version, -v       Show version info
  --check-env         Check terminal environment compatibility
  --auto-login=USER   Skip login, log in as USER directly
  --user=USER         Login as USER (uses --password for auth)
  --password=PASS     Password for --user
  --config=FILE       Specify config file path
  --theme=NAME        Specify theme name
  --debug             Enable debug logging
  --no-mouse          Disable mouse support
  --force-mouse       Force enable mouse
  --reset-config      Reset config to defaults

Examples:
  tdesktop
  tdesktop --debug --theme=dark
  tdesktop --check-env
)";
    std::cout << help << std::endl;
}

static void checkEnvironment() {
    TermDetect detector;
    TermInfo info = detector.detect();
    std::cout << info.summary() << std::endl;
    std::cout << "\nStatus: " << (info.score() >= 50 ? "OK - Can run" : "WARNING - May have issues") << std::endl;
}

static bool checkRoot() {
    return geteuid() == 0;
}

// Verify password by running `su -c true user` via forkpty.
// Works for non-root callers authenticating as any user.
static bool verifyViaSu(const std::string& user, const std::string& pass) {
    int master;
    pid_t pid = forkpty(&master, nullptr, nullptr, nullptr);
    if (pid < 0) {
        LOG_ERROR("forkpty failed");
        return false;
    }

    if (pid == 0) {
        // Child: ensure su is found, use explicit /bin/su
        execl("/bin/su", "su", user.c_str(), "-c", "true", nullptr);
        // If execl fails
        _exit(2);
    }

    // Give su a moment to start and prompt for password
    usleep(300000);
    std::string input = pass + "\n";
    [[maybe_unused]] ssize_t _ = write(master, input.data(), input.size());

    int status;
    pid_t w = waitpid(pid, &status, 0);
    ::close(master);

    if (w < 0) {
        LOG_ERROR("waitpid failed");
        return false;
    }
    if (!WIFEXITED(status)) {
        LOG_WARN("su did not exit normally, signal=" + std::to_string(WTERMSIG(status)));
        return false;
    }
    int code = WEXITSTATUS(status);
    return code == 0;
}

int main(int argc, char* argv[]) {
    Options opts = parseArgs(argc, argv);

    if (opts.show_help) {
        printHelp();
        return 0;
    }

    if (opts.check_env) {
        checkEnvironment();
        return 0;
    }

    // Initialize logger
    Logger::instance().init("/tmp/tdesktop.log");
    Logger::instance().setDebug(opts.debug);
    if (opts.debug) Logger::instance().setLevel(LogLevel::Debug);

    LOG_INFO("TDESKTOP starting...");

    // Setup signal handlers
    SignalHandler::instance().setup();

    // Initialize terminal
    Terminal term;
    if (!term.init()) {
        LOG_ERROR("Failed to initialize terminal");
        return 1;
    }

    // Ensure terminal is restored on crash/exit
    SignalHandler::instance().onShutdown([&term]() {
        term.restore();
        Logger::instance().info("TDESKTOP shutdown complete");
    });

    // Initialize color manager
    ColorManager colors;

    // Initialize event loop
    EventLoop loop;
    loop.init(STDIN_FILENO);

    // Initialize config
    ConfigManager config;
    if (opts.reset_config) {
        config.createDefault(config.findConfigPath());
    } else {
        config.load(opts.config_path);
    }

    // Apply config overrides
    if (!opts.theme.empty()) config.getConfig().theme = opts.theme;
    if (opts.no_mouse) config.getConfig().mouse_enabled = false;

    // Login screen
    LoginResult login_result = LoginResult::Success;
    std::string username;

    LoginScreen login(term, colors);
    PAMAuth pam;
    ShadowAuth shadow;
    bool use_shadow = false;

    bool pam_ok = pam.init();
    if (pam_ok) {
        LOG_INFO("Using PAM authentication");
    } else if (shadow.isAvailable()) {
        use_shadow = true;
        LOG_INFO("PAM unavailable, using shadow authentication");
    } else {
        LOG_ERROR("No authentication method available (PAM config missing, shadow not readable)");
        std::cerr << "No authentication method available." << std::endl;
        term.restore();
        return 1;
    }

    if (opts.auto_login) {
        login_result = LoginResult::Success;
        username = opts.auto_user;
        LOG_INFO("Auto-login as: " + username);
    } else if (opts.cli_user) {
        username = opts.auto_user;
        bool auth_ok = verifyViaSu(username, opts.cli_password);
        if (auth_ok) {
            LOG_INFO("User logged in: " + username);
        } else {
            term.restore();
            LOG_ERROR("Authentication failed for user: " + username);
            std::cerr << "Authentication failed." << std::endl;
            return 1;
        }
    } else {
        int attempts = 0;
        do {
            login_result = login.run();

            if (login_result == LoginResult::Quit) {
                term.restore();
                return 0;
            }

            if (login_result == LoginResult::Success) {
                username = login.getUsername();
                std::string password = login.getPassword();
                attempts++;

                bool auth_ok = false;
                if (use_shadow) {
                    auth_ok = shadow.authenticate(username, password);
                } else {
                    auth_ok = pam.authenticate(username, password);
                }
                if (!auth_ok) {
                    LOG_WARN("Primary auth failed for: " + username + ", trying su fallback");
                    auth_ok = verifyViaSu(username, password);
                }

                if (auth_ok) {
                    LOG_INFO("User logged in: " + username);
                    break;
                }

                login_result = LoginResult::Failed;
                LOG_WARN("Authentication failed for user: " + username);
                login.setError("Authentication failed. Try again.");
            }
        } while (attempts < 3);

        if (login_result != LoginResult::Success) {
            term.restore();
            std::cerr << "Authentication failed after 3 attempts." << std::endl;
            return 1;
        }
    }

    // Switch to user if running as root
    if (checkRoot() && !opts.auto_login && username != "root") {
        struct passwd* pw = getpwnam(username.c_str());
        if (pw) {
            setenv("HOME", pw->pw_dir, 1);
            setenv("USER", pw->pw_name, 1);
            setenv("LOGNAME", pw->pw_name, 1);
            setenv("SHELL", pw->pw_shell ? pw->pw_shell : "/bin/bash", 1);
        }
    }

    // Hide cursor
    term.showCursor(false);
    term.clearScreen();

    // Initialize desktop
    Desktop desktop(term, loop, colors);
    if (!desktop.init()) {
        term.restore();
        LOG_ERROR("Failed to initialize desktop");
        return 1;
    }

    // Setup notification manager
    NotifyManager notify_mgr;
    DbusListener dbus(notify_mgr);
    dbus.start();

    // Show welcome notification
    notify_mgr.show("Welcome", "TUI Desktop Environment v1.0.0", NotifyUrgency::Normal);

    // Run desktop
    LOG_INFO("Desktop running...");
    desktop.run();

    // Cleanup
    dbus.stop();
    desktop.shutdown();
    term.restore();
    LOG_INFO("TDESKTOP exited normally");

    return 0;
}
