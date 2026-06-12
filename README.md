# TUI Desktop Environment (TDESKTOP)

A graphical desktop environment that runs in your terminal — no X11/Wayland required.

## Quick start

```sh
# Dependencies: cmake g++ libpam0g-dev
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
./bin/tdesktop
```

## Features

- Window management (move, resize, focus, close)
- Taskbar with window list and clock
- Application launcher with search
- PAM / shadow authentication
- Built-in command panel (F12)
- Notification popups (D-Bus listener)
- Mouse support (hardware + terminal protocol)
- Multi-workspace (up to 9)
- Color themes (default, dark, hacker)
- JSON configuration

## Docs

- `docs/user-guide.md` — usage instructions
- `docs/keybindings.md` — keyboard shortcuts
- `docs/faq.md` — troubleshooting

## Install

```sh
sudo ./scripts/install.sh
```

## License

GPLv3
