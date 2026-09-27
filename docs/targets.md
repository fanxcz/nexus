# Targets

Nexus separates the compiler host from the generated-program target.

## Implemented

- `x86_64-linux`: native v0.1 target tested on Debian 13.

## Planned

- `aarch64-linux`
- `x86_64-windows-gnu`
- `x86_64-windows-msvc`
- `aarch64-windows`
- `x86_64-macos`
- `aarch64-macos`
- `aarch64-android`
- `wasm32-wasi`
- freestanding `x86_64-none`, `aarch64-none`, `riscv64-none`

A target is only marked supported after its compiler/toolchain, runtime, linker and CI path are tested. The frontend is not tied to Linux.
