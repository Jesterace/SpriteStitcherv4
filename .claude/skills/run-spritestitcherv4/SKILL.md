---
name: run-spritestitcherv4
description: Build, launch, and drive the SpriteStitcher v4 Qt6 desktop app (import a sprite, edit a cross-stitch pattern, export a Pattern Keeper-compatible PDF). Use when asked to run SpriteStitcher, start the app, build it, take a screenshot of its UI, test the import/export flow, or click through the desktop app.
---

SpriteStitcher v4 is a native Qt6 Widgets desktop app (C++, not
Electron/web) — no browser, no Node. Drive it via the `xdotool` +
`spectacle` wrapper at `.claude/skills/run-spritestitcherv4/driver.sh`.
This machine has a real X11/Wayland desktop (not a headless Xvfb
container), so the driver controls the actual live display.

All paths below are relative to the repo root.

## Prerequisites

```bash
sudo pacman -S cmake qt6-base libharu xdotool   # Arch
# sudo apt-get install -y cmake qt6-base-dev libhpdf-dev xdotool   # Debian/Ubuntu
```

`spectacle` (KDE's screenshot tool) is also required for the
`screenshot` driver command; it ships with Plasma by default.

## Build

```bash
.claude/skills/run-spritestitcherv4/driver.sh build
```

Configures into `build/` (Ninja, RelWithDebInfo) and builds. Safe to
re-run — it's an incremental build.

## Run (agent path)

The driver is a stateless bash CLI (no REPL/tmux needed — `xdotool`
talks directly to the running X server, so there's no in-process state
to keep alive between commands). It tracks the app's PID and main
window ID in `/tmp/spritestitcher-driver/` between invocations.

```bash
D=.claude/skills/run-spritestitcherv4/driver.sh
$D launch                          # idempotent — no-ops if already running
$D screenshot /tmp/shots/01.png
```

A full import → edit-ready → PDF-export pass, exactly as verified
while building this skill:

```bash
D=.claude/skills/run-spritestitcherv4/driver.sh
$D launch

# Open Image... is at (103, 101) in the default 1400x919 layout this
# app opens at (window moved to 50,50 by `launch`).
$D click 103 101
DLG=$($D wait-window "Open Sprite Image" 10)
$D activate "$DLG"
$D click 855 660                   # the Name: field
$D type "$(pwd)/samples/test_sprite.png"
$D key Return                      # submits the dialog — see Gotchas

DLG=$($D wait-window "Import Options" 10)
$D activate "$DLG"
$D key Return                      # accepts the default (Exact colors)

$D screenshot /tmp/shots/pattern-loaded.png   # verify: grid + palette populated

$D click 304 101                   # Export PDF... in the toolbar
DLG=$($D wait-window "Export PDF Chart" 10)
$D activate "$DLG"
$D click 855 628                   # the Name: field (pre-filled with the pattern name)
$D key ctrl+a
$D type "/tmp/shots/out.pdf"
$D key Return

$D quit
```

`samples/test_sprite.png` is a small 20x20, 5-color fixture committed
in the repo for exactly this purpose.

### Commands

| command | what it does |
|---|---|
| `build` | cmake configure + build into `build/` |
| `launch` | launch the app if not already running; moves/raises/activates the main window; prints `window=<id>` |
| `window` | print the tracked main window id |
| `windows` | list every named top-level window currently open — use to find a dialog that just appeared |
| `wait-window <name-substring> [timeout-s]` | poll until a window matching the name appears; prints its id (default timeout 10s) |
| `activate <id>` | raise + focus a window |
| `click <x> <y>` / `rclick <x> <y>` | move mouse to absolute screen coords and left/right-click |
| `type <text>` | type text into whatever has focus |
| `key <keysym>` | send a key, e.g. `Return`, `ctrl+a` |
| `screenshot <path>` | full-screen screenshot via `spectacle -b -n -o` |
| `quit` | kill the app process, clear tracked state |

## Run (human path)

```bash
./build/app/spritestitcher   # opens a normal window; close it to quit
```

## Gotchas

- **File dialogs are invisible to `xdotool` unless you force XWayland.**
  This is a KDE Plasma Wayland session; Qt's native/portal file dialogs
  render as Wayland surfaces that `xdotool search` never finds — clicks
  and keystrokes go nowhere. The driver's `launch` command sets
  `QT_QPA_PLATFORM=xcb` (forces the whole app onto XWayland) **and**
  `QT_QPA_PLATFORMTHEME=` empty (avoids a KDE-native dialog theme that
  can still render as a non-X11 surface even under `xcb`). Both are
  required — either alone still leaves dialogs unclickable.

- **Clicking a dialog's default button (Open/OK) at its exact
  coordinates sometimes silently does nothing**, even after
  `activate` and verified-correct coordinates — not consistently
  reproducible, plausibly a focus/compositor timing race. Sending
  `key Return` instead reliably submits the dialog every time. Prefer
  `key Return` over clicking Open/OK/Save; only use coordinate clicks
  for things that aren't a dialog's default action (toolbar buttons,
  the Name: field itself, palette rows).

- **The window name search matches more than the main window.** This
  app creates a couple of tiny (1x1 / 3x3) internal Qt windows that
  also match `xdotool search --name "SpriteStitcher"` (e.g. "Qt
  Selection Owner for spritestitcher" doesn't match, but a stray 1x1
  "SpriteStitcher" window does). `launch` filters by `WIDTH > 100` via
  `getwindowgeometry --shell` to find the real one — reuse that
  pattern if you ever need to re-find the main window manually instead
  of using the `window`/`launch` state file.

- **The toolbar disables Save Project / Export PDF until a pattern is
  loaded.** If a click on Export PDF does nothing, you skipped the
  import step — `windows` will show no new dialog appeared.

## Troubleshooting

- **`wait-window` times out**: the triggering click likely landed on
  the wrong coordinates (window wasn't at the position you assumed —
  re-run `launch`, which always moves the window to `50,50` first) or
  the previous dialog is still open and modal. Run `windows` to see
  what's actually on screen, then `screenshot` to look at it.
- **`launch` times out waiting for the main window**: check
  `/tmp/spritestitcher-driver/app.log` for a crash/missing-library
  error. Confirm `build/app/spritestitcher` exists (`driver.sh build`).
- **Binary not found**: run `driver.sh build` first; the driver
  doesn't build automatically.
