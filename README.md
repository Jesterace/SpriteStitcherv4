# SpriteStitcher v4

Converts pixel art / sprite images into cross-stitch patterns, with
Pattern Keeper-compatible PDF chart export.

Status: feature-complete for its core path — image import, color
reduction, DMC matching, pattern data model, zoomable/pannable grid view,
sprite reference panel, DMC palette dock, Pattern Keeper-compatible PDF
chart export, cell recoloring, project-wide color swap, full undo/redo,
and native JSON project save/load. Remaining: a compiler-warnings cleanup
pass.

## Dependencies

- CMake 3.21+
- A C++17 compiler
- Qt 6 (Core, Gui, Widgets)
- [libharu](http://libharu.org/) (PDF export)

On Arch Linux:

```sh
sudo pacman -S cmake qt6-base libharu
```

On Debian/Ubuntu:

```sh
sudo apt install cmake qt6-base-dev libhpdf-dev
```

## Building

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

## Layout

- `core/` — UI-independent library (QtCore/QtGui only): image loading,
  color reduction, DMC color matching, pattern data model, project
  save/load, PDF export.
- `ui/` — Qt Widgets layer: `GridView` (custom-painted, viewport-culled
  stitch grid with cursor-anchored zoom/pan), `SpritePreviewPanel`
  (reference dock), `ToolPanel` (DMC palette dock), `MainWindow`.
- `app/` — the `spritestitcher` executable (thin `main.cpp`).
- `tools/dmc_gen/` — one-off script that generated the compiled-in DMC
  floss table (`core/src/dmc/DmcTableData.cpp`) from source data; not
  part of the build.

## Running

```sh
./build/app/spritestitcher
```

Use "Open Image..." to import a PNG/JPG/BMP sprite; you'll be asked
whether to match colors exactly or reduce to a target color count first.

Editing: click a row in the Palette dock to pick the active color, then
left-click a grid cell to paint it; right-click a cell to pick up its
color as the new active color (eyedropper). Use the Swap section to
replace one DMC color with another across the whole pattern. Undo/Redo
are in the Edit menu (Ctrl+Z / Ctrl+Shift+Z).

Project files (`.sstitch`, JSON) save the complete pattern state — grid,
DMC/symbol assignments, and the originally imported sprite image — via
File > Save Project. This is separate from PDF export, which is
output-only and isn't meant to round-trip back into an editable project.

## PDF export

`core/src/pdf/` renders charts with [libharu](http://libharu.org/) using
**only** base-14 Helvetica with a single-byte `StandardEncoding`font —
never a CID/Identity-H (two-byte) font path, and never Qt's own PDF
facilities. This is a hard requirement: earlier attempts using
`QPdfWriter`'s default CID/Identity-H text encoding produced PDFs that
Pattern Keeper could not parse. `core/tests/pdf_export_test.cpp` asserts
this directly by grepping the generated PDF bytes for `Identity-H`,
`/Type0`, and `CIDFontType` and failing if any are present.

Small patterns render as a single page (header + chart + DMC legend).
Patterns too large to stay legible at a 10pt minimum cell size on one
page are automatically tiled across multiple numbered pages, with a
dedicated info page (header + full legend) first.

## DMC color data

`core/src/dmc/DmcTableData.cpp` is generated from the 454-color dataset
published by the [`sharlagelfand/dmc`](https://github.com/sharlagelfand/dmc)
R package (itself sourced from `adrianj/CrossStitchCreator`), including
that package's documented corrections for a handful of typo'd/mismatched
entries. See `tools/dmc_gen/generate_dmc_table.py` for the exact
transform; it's not run as part of the build.
