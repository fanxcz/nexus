## 0.3.1

- Fixed chained `else if` parsing.
- Added `examples/nexus_arena.nx`, a full language showcase combining structs, enums, match, arrays, pointers, floating point, functions, loops and LLVM native code generation.

## v0.3.0 - 2026-09-27

- Added fixed-size arrays and array literals.
- Added safe runtime bounds checking for variable indices.
- Added index assignment.
- Added unit enums and enum variant expressions using `Enum::Variant`.
- Added `match` with `=>` arms and wildcard `_`.
- Added `else if` parsing.
- Added regression tests and examples for the new syntax.
- Kept Linux x86_64 as the validated native target; target abstraction remains cross-platform.

# Changelog

## 0.2.1 — String Operations Fix

- Fixed `string + string` semantic checking and LLVM code generation.
- Added runtime string concatenation.
- Added runtime string equality for `==` and `!=`.
- Tightened invalid string comparison diagnostics.
- Added regression tests for string concatenation and equality.

## 0.2.0 — Global Update

- Added `f64` numeric literals and arithmetic.
- Added structs, field access, struct literals and mutable field assignment.
- Added pointer/address-of/dereference operations for low-level programming.
- Added `break` and `continue`.
- Added relative source imports and project entry resolution from `nexus.toml`.
- Added external function declarations (`extern "C" fn ...`).
- Added target selection and target listing.
- Added project scaffolding through `nexus new` and `nexus init`.
- Added installable runtime discovery independent of the current working directory.
- Improved parser diagnostics with source positions.
- Added a dedicated standard-library directory layout.
