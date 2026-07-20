# Manual / End-to-End Testing Notes

`core/tests/` covers the library logic (DMC matching, color reduction,
pattern model, PDF export internals, undo, project serialization). This
file records findings from driving the actual `spritestitcher` binary
through its real UI — not a code-level bypass — since that's the only way
to catch UI-wiring bugs the unit tests can't see (dialog flow, signal/slot
connections, on-screen rendering).

## Environment setup for GUI automation

Dev machine is KDE Plasma on Wayland. Qt's native/portal file dialogs are
Wayland surfaces and are invisible to X11 tools like `xdotool` by default.
To drive the app with `xdotool`:

```sh
# xdotool: sudo pacman -S xdotool (or apt install xdotool)
QT_QPA_PLATFORM=xcb QT_QPA_PLATFORMTHEME= ./build/app/spritestitcher
```

- `QT_QPA_PLATFORM=xcb` forces the whole app — including its file dialogs
  — to run as XWayland/X11 clients, so `xdotool search`/`click` can see
  and interact with them.
- `QT_QPA_PLATFORMTHEME=` (empty) avoids picking up a KDE-native dialog
  implementation that can still render as a non-X11 surface; this
  guarantees the standard cross-platform Qt file dialog appears instead.
- Without both, `xdotool search` finds the main window fine but never
  finds file dialogs — they open as native Wayland portal windows outside
  X11's visibility, and clicks/keystrokes sent to the main window go
  nowhere useful while a dialog most users would see is actually open.

## Known quirk: click vs. Enter on dialog buttons

`xdotool click` at a dialog button's exact on-screen coordinates
(Open/OK in the file-open and Import Options dialogs) sometimes did not
register, even after `windowactivate` and verified-correct coordinates.
Pressing `Return` with focus already on the relevant field reliably
submitted the dialog every time. Root cause not fully diagnosed (plausibly
a focus/compositor timing race between `xdotool windowactivate`,
`click`, and KDE's dialog rendering) — not an application defect. The
underlying `QDialogButtonBox` default-button wiring works correctly in
both `ImportOptionsDialog` and the native `QFileDialog`; when in doubt,
prefer `xdotool key Return` over clicking a dialog's default button.

## What was verified (2026-07-20)

Full manual pass through the real UI on the live display:

1. File > Open Image → native file dialog → typed a path to a 20x20 test
   sprite → Open
2. Import Options dialog → "Exact colors" (default) → OK
3. Pattern rendered correctly in `GridView` (colors, symbols, grid lines,
   red center lines, ruler numbers every 10 stitches) and `ToolPanel`
   (DMC palette with correct swatches/codes/names/counts); status bar
   ("20 x 20 stitches — 400 total — 5 colors") and window title
   ("test_sprite — SpriteStitcher") updated correctly
4. Export PDF → save dialog correctly pre-filled the suggested filename
   from the pattern name → saved
5. Verified the output PDF:
   - `pdfinfo`/`strings`: `/BaseFont /Helvetica` and `/Helvetica-Bold`,
     `/Encoding /StandardEncoding` — no `Type0`/`CIDFontType`/
     `Identity-H` anywhere (the hard Pattern Keeper compatibility
     requirement)
   - Rendered to an image (`pdftoppm`) and confirmed it matches the
     on-screen pattern exactly: colors, symbols, grid, center lines,
     ruler, header (name/dimensions/fabric estimates), and DMC legend

No crashes, hangs, or rendering issues encountered. Save/Export toolbar
actions correctly went from disabled to enabled after import, matching
`MainWindow`'s intended state management.
