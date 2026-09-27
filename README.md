# NEXUS

Nexus is a universal programming language project built with C++20 and LLVM. The compiler is developed on Debian Linux, while the language architecture separates host and target so that native targets can be added without changing the core language.

## Current status — v0.1.0

Working foundation:

- C++20 compiler
- lexer
- recursive-descent / precedence parser
- AST
- semantic/type checking
- variables
- i64 / bool / string
- functions and recursion
- if/else
- while
- arithmetic/comparison/logical expressions
- native LLVM IR generation
- Clang/LLVM native linking
- CLI: build/run/check/version/help
- tests

Initial native target: Linux x86_64.

The target layer is intentionally separate so Windows, macOS, Linux ARM64, Android and WebAssembly can be added next.

## Build on Debian 13

```bash
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

You need C++20, CMake, Ninja and an LLVM-compatible Clang. This environment can use Clang 17 to consume the generated `.ll` files. Future versions can switch the backend to LLVM C++ APIs when LLVM development headers are installed.

## Run

```bash
./build/nexus version
./build/nexus check examples/hello.nx
./build/nexus build examples/hello.nx -o hello
./hello
```

Or:

```bash
./build/nexus run examples/hello.nx
```

Emit LLVM IR:

```bash
./build/nexus build examples/hello.nx --emit-ir
```

## Cross-platform design

Nexus separates:

- host platform
- target triple
- ABI
- linker/toolchain
- runtime platform layer

The frontend is target-independent. Linux x86_64 is the first tested target; Windows, macOS, ARM64, Android, WebAssembly and freestanding targets are planned and must only be marked supported after real CI/build tests pass.

## Roadmap

- v0.1: frontend + native LLVM IR backend + Linux x86_64
- v0.2: structs/enums/modules/package manager/C FFI
- v0.3: Windows + Linux ARM64 + concurrency foundations
- v0.4: macOS + Android + WebAssembly/WASI
- v1.0: stable language and verified multi-target toolchains

## Repository

This project is intended for GitHub under the `fanxcz` account as the `nexus` repository.
