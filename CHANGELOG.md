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
