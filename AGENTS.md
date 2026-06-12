# TDESKTOP — Agent guide

## Build & test

```sh
# Configure & build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

# Build with tests
cmake .. -DBUILD_TESTING=ON && make -j$(nproc)

# Run the only existing test
make test_buffer && ./tests/unit/test_buffer
```

PAM (`libpam0g-dev`) is the only hard build dependency. If missing, PAM::PAM becomes a no-op stub and `shadow_auth.cpp` falls back to `/etc/shadow`. The custom `cmake/FindPAM.cmake` is used.

## Architecture

```
main.cpp → terminal init → signal handlers → config → login → desktop → event loop
```

All code lives under `tdesktop` namespace. `using namespace tdesktop;` is used in `main.cpp`.

### Entrypoint: `src/main.cpp`

Startup sequence:
1. Parse CLI args (`--debug`, `--auto-login=USER`, `--theme=NAME`, `--no-mouse`, `--check-env`, etc.)
2. Logger writes to `/tmp/tdesktop.log`
3. `SignalHandler` traps SIGINT/SIGTERM → restores terminal on crash
4. `Terminal::init()`: raw mode + alternate screen
5. Config loaded from `~/.config/tdesktop/tdesktop.conf` (simple hand-rolled JSON parser)
6. Login: tries PAM → falls back to shadow auth. Root skips login. 3 attempts max.
7. Desktop init → run loop (16ms poll) → shutdown → terminal restore

### Login quirk

`LoginScreen::run()` has its **own inner event loop**. When the user presses Enter on the password field it sets `input_field_ = 2` and returns `LoginResult::Success`. Actual PAM/shadow authentication happens back in `main()`, not inside `LoginScreen`.

### Event dispatch priority

1. Command panel (if open)
2. Launcher (if visible)
3. `KeyboardHandler` hotkeys

Hotkeys are registered in `Desktop::setupHotkeys()` via `bindHotkey(Key, ctrl, alt, shift, callback)`.

### Modules

| Directory | Purpose | Key classes |
|---|---|---|
| `core/` | Terminal control, double-buffer, event loop, renderer, color | `Terminal`, `Buffer`, `EventLoop`, `Renderer`, `ColorManager` |
| `input/` | Keyboard hotkeys, mouse (dual-mode), terminal detection | `KeyboardHandler`, `MouseHandler`, `TermDetect` |
| `desktop/` | Window manager, taskbar, launcher, workspaces, cmd panel | `WindowManager`, `Taskbar`, `Launcher`, `WorkspaceManager`, `CommandPanel` |
| `auth/` | PAM + shadow authentication, login UI | `PAMAuth`, `ShadowAuth`, `LoginScreen` |
| `widgets/` | Reusable TUI controls | `Button`, `TextBox`, `ListBox`, `Menu` |
| `notification/` | D-Bus listener in background thread, notification popups | `NotifyManager`, `DbusListener` |
| `utils/` | Config (JSON), logger, UTF helpers, signal handler | `ConfigManager`, `Logger`, `SignalHandler` |

## Known quirks / gotchas

- **`Buffer::render()`** uses `const_cast` to merge dirty rects — logically const, but modifies mutable cache.
- **`CommandPanel`** has `close()` method that collides with POSIX `::close()`. Must qualify with `::close(fd)` in implementation.
- **Mouse**: dual-mode (hardware `/dev/input/mice` or terminal SGR 1006 protocol). Auto-detects, auto-fallbacks.
- **`forkpty`** include: `<pty.h>` on Linux, `<util.h>` on other Unix.
- **`helpers.h`**: `struct sigaction` (must include `<signal.h>`, not `<csignal>`). On some platforms the unqualified `sigaction` fails.
- **New files** commonly miss: `<memory>` (unique_ptr), `<chrono>`, `<unistd.h>` (write/STDOUT_FILENO), `<signal.h>`.
- **Config**: hand-rolled JSON parser in `ConfigManager::parseJson()` — no nlohmann or external dep. Only reads basic string/number/bool/array values.
- **Tests**: only 1 test (`test_buffer`). The test CMakeLists directly `#include`s `../../src/core/buffer.cpp` instead of linking a library target.
- **PAM auth**: uses a custom `pamConv` callback that passes password via `PamData` struct. Service name is `"tdesktop"` (expects `/etc/pam.d/tdesktop`).
