# Changelog

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
