#!/usr/bin/env bash
# Driver for building, launching, and driving the SpriteStitcher v4 Qt
# desktop app via xdotool + spectacle on a real X11/Wayland desktop.
# See SKILL.md in this directory for usage and the commands below.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
BIN="$BUILD_DIR/app/spritestitcher"
STATE_DIR="${SPRITESTITCHER_DRIVER_STATE:-/tmp/spritestitcher-driver}"
PID_FILE="$STATE_DIR/pid"
WIN_FILE="$STATE_DIR/window_id"
LOG_FILE="$STATE_DIR/app.log"

mkdir -p "$STATE_DIR"
export DISPLAY="${DISPLAY:-:0}"

cmd="${1:-}"
shift || true

case "$cmd" in
  build)
    cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build "$BUILD_DIR"
    ;;

  launch)
    if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
      echo "already running (pid $(cat "$PID_FILE"), window $(cat "$WIN_FILE" 2>/dev/null || echo '?'))"
      exit 0
    fi
    [ -x "$BIN" ] || { echo "binary not found at $BIN — run '$0 build' first" >&2; exit 1; }

    # QT_QPA_PLATFORM=xcb forces the app (and its file dialogs) onto
    # XWayland so xdotool can see them at all. QT_QPA_PLATFORMTHEME=
    # (empty) avoids the KDE-native dialog theme, which can still render
    # as a non-X11 portal surface even under xcb. Without both, dialogs
    # are invisible to xdotool. See SKILL.md Gotchas.
    nohup env QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME= "$BIN" > "$LOG_FILE" 2>&1 &
    echo $! > "$PID_FILE"
    disown || true

    # The main window's title matches "SpriteStitcher" but so do a few
    # tiny (1x1 / 3x3) internal Qt selection-owner windows this app
    # creates — filter by geometry to find the real one.
    for _ in $(seq 1 30); do
      sleep 0.2
      win=""
      for w in $(xdotool search --name "SpriteStitcher" 2>/dev/null || true); do
        width="$(xdotool getwindowgeometry --shell "$w" 2>/dev/null | grep '^WIDTH=' | cut -d= -f2 || true)"
        if [ -n "$width" ] && [ "$width" -gt 100 ]; then
          win="$w"
          break
        fi
      done
      if [ -n "$win" ]; then
        echo "$win" > "$WIN_FILE"
        xdotool windowmove "$win" 50 50
        xdotool windowraise "$win"
        xdotool windowactivate "$win"
        echo "launched: pid=$(cat "$PID_FILE") window=$win"
        exit 0
      fi
    done
    echo "timed out waiting for the main window — check $LOG_FILE" >&2
    exit 1
    ;;

  window)
    [ -f "$WIN_FILE" ] && cat "$WIN_FILE" || { echo "no tracked window — run '$0 launch' first" >&2; exit 1; }
    ;;

  windows)
    # Every named top-level window currently open. Use this after a
    # click to find a dialog that just appeared (file chooser, Import
    # Options, Export PDF, unsaved-changes prompt, ...).
    for w in $(xdotool search --name "" 2>/dev/null || true); do
      name="$(xdotool getwindowname "$w" 2>/dev/null || true)"
      [ -n "$name" ] && echo "$w: $name"
    done
    ;;

  activate)
    win="${1:?usage: driver.sh activate <window_id>}"
    xdotool windowactivate "$win"
    ;;

  click)
    x="${1:?usage: driver.sh click <x> <y>}"
    y="${2:?usage: driver.sh click <x> <y>}"
    xdotool mousemove "$x" "$y"
    sleep 0.2
    xdotool click 1
    ;;

  rclick)
    x="${1:?usage: driver.sh rclick <x> <y>}"
    y="${2:?usage: driver.sh rclick <x> <y>}"
    xdotool mousemove "$x" "$y"
    sleep 0.2
    xdotool click 3
    ;;

  type)
    text="${1:?usage: driver.sh type <text>}"
    xdotool type --delay 20 "$text"
    ;;

  key)
    keysym="${1:?usage: driver.sh key <keysym, e.g. Return>}"
    xdotool key "$keysym"
    ;;

  wait-window)
    # Poll until a window whose name contains <pattern> appears. Prints
    # its id. Useful right after triggering a dialog.
    pattern="${1:?usage: driver.sh wait-window <name-substring> [timeout-seconds]}"
    timeout_s="${2:-10}"
    end=$((SECONDS + timeout_s))
    while [ "$SECONDS" -lt "$end" ]; do
      for w in $(xdotool search --name "$pattern" 2>/dev/null || true); do
        echo "$w"
        exit 0
      done
      sleep 0.2
    done
    echo "timed out waiting for a window matching '$pattern'" >&2
    exit 1
    ;;

  screenshot)
    out="${1:?usage: driver.sh screenshot <output.png>}"
    sleep 0.3
    spectacle -b -n -o "$out"
    echo "$out"
    ;;

  quit)
    if [ -f "$PID_FILE" ]; then
      pkill -f "$BIN" 2>/dev/null || true
      rm -f "$PID_FILE" "$WIN_FILE"
    fi
    ;;

  *)
    echo "usage: $0 {build|launch|window|windows|activate|click|rclick|type|key|wait-window|screenshot|quit}" >&2
    exit 1
    ;;
esac
