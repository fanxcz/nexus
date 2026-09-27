# NEXUS

NEXUS is a universal programming language with a C++20 compiler, LLVM-compatible native code generation, and a growing cross-platform runtime. The compiler is developed on Debian 13, while the language frontend is designed to stay platform-independent.

## v0.4.0 — Interactive Apps & Games Runtime

NEXUS can now be used to build genuinely interactive command-line applications and terminal games instead of programs that only print a fixed sequence of output.

Working now:

- `input`, `input_i64`, `input_f64`
- `str_i64`, `str_f64`, `str_bool`
- `sleep`, `time_ms`, `random_i64`
- `clear`, `system`, `exit`, `beep`
- `file_read`, `file_write`, `file_exists`, `env`
- terminal game loop: `screen_begin/end/clear/put/present`
- non-blocking keyboard input: `key_pressed`, `read_key`
- terminal size: `screen_width`, `screen_height`
- terminal title: `screen_set_title`
- project templates: `nexus new --template cli|app|game`
- interactive CLI example
- playable terminal-game example
- `nexus_studio.nx`: calculator, guess game, mini battle, terminal game and runtime diagnostics
- parser fix for simple conditions such as `while running { ... }`

## Existing language features

- C++20 compiler
- lexer + recursive-descent / precedence parser
- AST + semantic/type checking
- `i64`, `f64`, `bool`, `string`, pointers and user-defined structs
- functions and recursion
- `if` / `else if` / `else`, `while`, `break`, `continue`
- arithmetic, comparison and logical expressions
- mutable variables and mutable struct fields
- fixed-size arrays with bounds checking
- unit enums and `match`
- LLVM IR generation and native linking through Clang
- local source imports
- `nexus.toml`
- `nexus new` / `nexus init`
- target selection and `nexus targets`
- C FFI declarations
- CMake, tests and GitHub CI

Still experimental / planned:

- enum payloads and full generic types
- package registry and dependency solving
- HIR/MIR compiler layers
- LSP
- full async runtime
- verified native cross-compilation toolchains for every advertised target
- graphical backend (the current game API is terminal-native)

## Build on Debian 13

```bash
cmake -S . -B build -G Ninja
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

You need C++20, CMake, Ninja and an LLVM-compatible Clang. The current Debian development environment uses Clang to consume generated LLVM IR.

## First program

```bash
./build/nexus new hello --template cli
cd hello
../nexus/build/nexus run
```

Direct compilation:

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

## Create an application

```bash
./build/nexus new myapp --template app
cd myapp
../nexus/build/nexus run
```

The generated app already has input, a menu, calculator logic and runtime calls.

## Create a game

```bash
./build/nexus new mygame --template game
cd mygame
../nexus/build/nexus build
../nexus/build/nexus run
```

The generated game supports:

- WASD movement
- live keyboard polling
- terminal rendering
- score
- frame timing
- clean terminal shutdown

## Interactive examples

### Full studio showcase

```bash
./build/nexus check examples/nexus_studio.nx
./build/nexus build examples/nexus_studio.nx -o nexus-studio
./nexus-studio
```

It contains:

1. calculator
2. number guessing game
3. mini turn-based battle
4. real-time terminal game
5. runtime diagnostics

### Save/load application

```bash
./build/nexus build examples/interactive_cli.nx -o interactive-cli
./interactive-cli
```

### Standalone terminal game

```bash
./build/nexus build examples/terminal_game.nx -o terminal-game
./terminal-game
```

Use `WASD` to move and `Q` to quit.

## Interactive runtime API

```nx
let name = input("Name: ")
let age = input_i64("Age: ")
print("Hello, " + name)
print(str_i64(age + 1))
```

Persistence:

```nx
file_write("save.txt", "hello")
let data = file_read("save.txt")
let exists = file_exists("save.txt")
let home = env("HOME")
```

Terminal game loop:

```nx
screen_begin()
let mut running = true
while running {
    screen_clear()
    screen_put(10, 5, "@")
    screen_present()

    if key_pressed() {
        let key = read_key()
        if key == 113 { running = false }
    }

    sleep(30)
}
screen_end()
```

## Targets

```bash
./build/nexus targets
```

The compiler exposes:

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

Target-specific native linking still depends on the toolchain/sysroot installed on the build machine. Linux x86_64 is the validated native development target in this repository.

## Repository

https://github.com/fanxcz/nexus
