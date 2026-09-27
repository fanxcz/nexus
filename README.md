# NEXUS

Nexus is a universal programming language built around a C++20 compiler and LLVM-compatible native code generation. The compiler is developed on Debian 13, while the language frontend and target model are designed to remain platform-independent.

## v0.2.0 — Global Update

Working now:

- C++20 compiler
- lexer + recursive-descent / precedence parser
- AST + semantic/type checking
- `i64`, `f64`, `bool`, `string`, pointers and user-defined structs
- functions, recursion and external function declarations
- `if` / `else`, `while`, `break`, `continue`
- arithmetic, comparison and logical expressions
- mutable variables and mutable struct fields
- LLVM IR generation and native linking through Clang
- source imports for local `.nx` modules
- project scaffolding with `nexus new` / `nexus init`
- `nexus.toml` project entry point
- target model and `nexus targets`
- installable runtime discovery
- CMake + tests + GitHub CI

Experimental / in progress:

- enums are parsed but not yet lowered to native code
- package registry and dependency solving are planned
- LSP, async runtime and full cross-platform native CI are planned

## Build on Debian 13

```bash
cmake -S . -B build -G Ninja
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

You need C++20, CMake, Ninja and an LLVM-compatible Clang. This tree uses Clang 17 in the current Debian development environment to consume generated LLVM IR.

## First program

```bash
./build/nexus new hello
cd hello
../nexus/build/nexus run
```

Or compile a file directly:

```bash
./build/nexus build examples/hello.nx -o hello
./hello
```

Check without linking:

```bash
./build/nexus check examples/hello.nx
```

Emit LLVM IR:

```bash
./build/nexus build examples/hello.nx --emit-ir
```

## Language examples

### Structs

```nx
struct Player {
    id: i64
    health: i64
    speed: f64
}

fn main() {
    let mut p = Player { id: 1, health: 100, speed: 4.5 }
    p.health = p.health - 10
    print(p.health)
}
```

### Pointers

```nx
fn main() {
    let mut x = 10
    let p = &x
    x = 42
    print(*p)
}
```

### Local modules

```nx
import "utility.nx"

fn main() {
    print(double(21))
}
```

## Targets

```bash
./build/nexus targets
```

The compiler exposes these target names:

- `native`
- `x86_64-linux`
- `aarch64-linux`
- `x86_64-windows`
- `aarch64-windows`
- `x86_64-macos`
- `aarch64-macos`
- `aarch64-android`
- `wasm32-wasi`

LLVM IR can be emitted for any configured target. Native linking only succeeds when the current machine has a compatible Clang target toolchain/sysroot.

## Repository

GitHub target: `https://github.com/fanxcz/nexus`
