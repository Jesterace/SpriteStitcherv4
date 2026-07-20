# SpriteStitcher v4

Converts pixel art / sprite images into cross-stitch patterns, with
Pattern Keeper-compatible PDF chart export.

Status: core library and grid UI working (image import, color reduction,
DMC matching, pattern data model, zoomable/pannable grid view, sprite
reference panel, DMC palette dock). PDF export and editing tools land in
later phases.

## Dependencies

- CMake 3.21+
- A C++17 compiler
- Qt 6 (Core, Gui, Widgets)
- [libharu](http://libharu.org/) (PDF export, once that phase lands)

On Arch Linux:

```sh
sudo pacman -S cmake qt6-base libharu
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

## DMC color data

`core/src/dmc/DmcTableData.cpp` is generated from the 454-color dataset
published by the [`sharlagelfand/dmc`](https://github.com/sharlagelfand/dmc)
R package (itself sourced from `adrianj/CrossStitchCreator`), including
that package's documented corrections for a handful of typo'd/mismatched
entries. See `tools/dmc_gen/generate_dmc_table.py` for the exact
transform; it's not run as part of the build.
