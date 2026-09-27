# Targets

Nexus separates the **host that runs the compiler** from the **target produced by the compiler**.

## Target names exposed by the CLI

```text
native
x86_64-linux
aarch64-linux
x86_64-windows
aarch64-windows
x86_64-macos
aarch64-macos
aarch64-android
wasm32-wasi
```

## Verified in this repository

- `native` / Linux x86_64: native executable build and run tested on Debian 13.
- `x86_64-windows`: LLVM IR target selection tested; native linking requires a Windows-compatible sysroot/toolchain that is not installed in the Debian development environment.

The remaining targets are accepted by the target layer and can emit target-specific LLVM IR, but they are **not claimed as fully supported native platforms** until their toolchain/runtime/CI paths are verified.

The language frontend itself is target-independent.
