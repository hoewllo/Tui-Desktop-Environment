# FAQ

## Mouse doesn't work in SSH
Use `tdesktop --no-mouse` or rely on keyboard navigation. Terminal mouse protocol should work in most modern terminals.

## Screen flickering
Run `tdesktop` in a terminal that supports 256 colors or better.

## How do I change the theme?
Edit `~/.config/tdesktop/tdesktop.conf` and set `"theme": "dark"` or `"theme": "hacker"`. Or use `tdesktop --theme=dark`.

## How do I exit?
Press `Ctrl+Q` or close the terminal.

## Can I customize key bindings?
Yes, edit the config file at `~/.config/tdesktop/tdesktop.conf`.
