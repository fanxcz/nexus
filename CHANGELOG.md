# Changelog

## v0.5.1
- Fixed graphics initialization so missing audio devices do not block SDL/OpenGL windows.
- Fixed SDL2 loader to treat BMP/WAV symbols as optional instead of required.
- Graphics diagnostics now report the actual SDL/OpenGL initialization error.
- Added `gfx_error()` diagnostics.
- Added cleanup on SDL/OpenGL initialization failures.
- Improved SDL2 runtime library discovery.
- Audio is now initialized lazily.

0.5.0 — Native Graphics, Audio and Events

- Added native SDL2/OpenGL window runtime loaded dynamically at application runtime.
- Added keyboard and mouse state, mouse coordinates and event inspection.
- Added 2D primitives: rectangles, circles, lines and built-in bitmap text.
- Added BMP texture loading/drawing/unloading.
- Added OpenGL 3D camera, grid and cube rendering.
- Added WAV audio loading/playback.
- Added runtime frame delta time and VSync control.
- Added `cli`, `app`, `game` and `3d` project templates.
- Added native application, native 2D game and native 3D examples.
- Fixed `return` from `main` without an explicit value in LLVM code generation.
- Improved parser diagnostics with source position for expression errors.


## 0.4.0 — Interactive Applications & Games

- Added blocking input builtins: `input`, `input_i64`, `input_f64`.
- Added keyboard/game-loop builtins: `screen_begin`, `screen_end`, `screen_clear`, `screen_put`, `screen_present`, `key_pressed`, `read_key`.
- Added runtime helpers for sleep, random numbers, monotonic time, system commands, terminal dimensions and terminal title.
- Added numeric/bool to-string conversions.
- Added portable file persistence and environment access.
- Added `nexus new --template cli|app|game`.
- Fixed parsing of simple boolean conditions before a block.
- Added interactive CLI and terminal-game examples.

## 0.3.1

- Fixed chained `else if` parsing.
- Added `examples/nexus_arena.nx`, a full language showcase combining structs, enums, match, arrays, pointers, floating point, functions, loops and LLVM native code generation.

## 0.3.0

- Added fixed-size arrays and array literals.
- Added safe runtime bounds checking for variable indices.
- Added index assignment.
- Added unit enums and enum variant expressions using `Enum::Variant`.
- Added `match` with `=>` arms and wildcard `_`.
- Added regression tests and examples for the new syntax.

## 0.2.1

- Fixed string concatenation and equality.
- Added runtime string helpers.

## 0.2.0

- Added structs, pointers, external functions, target selection, project scaffolding and runtime discovery.
