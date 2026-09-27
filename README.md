# NEXUS

## v0.9.0 — Real Project Workflow

NEXUS now treats a directory as a real project instead of requiring every command to name a `.nx` file.

New project tooling:
- `nexus new MyGame --template game` creates a complete project tree with `src/`, `assets/`, `scenes/`, and `nexus.toml`
- `nexus build` / `nexus run` / `nexus check` work without a source-file argument from inside a project
- `nexus info` prints manifest, target, entry, artifact and window configuration
- `nexus assets` inventories project assets and scenes
- `nexus clean` removes build output
- `nexus doctor` checks the native toolchain and runtime
- release builds go to `build/release/<artifact>`; debug builds go to `build/debug/<artifact>`
- installed NEXUS copies templates and starter assets with the compiler

Project layout:

```text
MyGame/
├── nexus.toml
├── src/
│   └── main.nx
├── assets/
│   ├── textures/
│   ├── audio/
│   ├── models/
│   └── fonts/
├── scenes/
│   └── main.nxs
└── build/
```

Example manifest:

```toml
[package]
name = "MyGame"
version = "0.9.0"
edition = "2026"

[build]
entry = "src/main.nx"
artifact = "MyGame"
target = "native"

[window]
width = 1280
height = 720
title = "MyGame"

[assets]
textures = "assets/textures"
audio = "assets/audio"
models = "assets/models"
fonts = "assets/fonts"
scenes = "scenes"
```

## Quick start

```bash
nexus new NeonGame --template game
cd NeonGame
nexus info
nexus assets
nexus check
nexus build
nexus run
```


## v0.8.1 — Normal Game Template and Reliable Audio

- `neon_survivor.nx`: centered playable top-down shooter
- procedural `gfx_sound_tone()` for guaranteed game SFX without external assets
- WAV output is converted to the active SDL audio format
- `nexus new <project> --template game` now uses the real game template

## v0.8.0 — Scenes, Sprite Animation, Physics and GUI Controls

NEXUS v0.8 adds a higher-level game/app layer on top of the native SDL2/OpenGL runtime.

New runtime APIs include:
- sprite animation handles with FPS, looping and frame queries
- texture filtering (nearest/linear)
- 2D AABB and circle collision helpers
- scene create/add/save/load/clear/destroy with a small deterministic `.nxs` scene format
- GUI checkbox and slider widgets
- existing ECS, particles, audio, fonts, textures, models and 2D/3D rendering remain available

Example APIs:

```nx
let scene = scene_create()
let player = ecs_create()
ecs_set_position(player, 120.0, 180.0)
scene_add(scene, player)
scene_save(scene, "save.nxs")

if physics_aabb(120.0, 180.0, 48.0, 48.0, 200.0, 180.0, 48.0, 48.0) {
    print("collision")
}
```

NEXUS is a universal programming language with a C++20 compiler, LLVM-compatible native code generation, and a growing cross-platform runtime. The compiler is developed on Debian 13, while the language frontend is designed to stay platform-independent.

## v0.7.0 — Native Game/App Runtime + Reliable Audio

### Audio that does not require SDL2_mixer

NEXUS now parses standard PCM WAV files itself and queues them through the core SDL2 audio device. This avoids relying on the optional `SDL_LoadWAV` or SDL2_mixer symbols that may not be exported by the installed SDL2 runtime.

Example:

```nx
let sound = gfx_sound_load("examples/assets/beep.wav")
if sound == 0 {
    print(gfx_audio_error())
    return
}
gfx_sound_volume(1.0)
gfx_sound_play(sound, false)
sleep(400)
gfx_sound_stop()
```

For MP3/OGG music, SDL2_mixer remains an optional backend. WAV always uses the core NEXUS audio path.

### v0.7 features

- reliable PCM WAV audio on Debian/Linux without SDL2_mixer
- audio availability and error reporting
- sound volume and queued-audio state
- lightweight OBJ model loading/drawing
- particle creation, update, draw and lifetime control
- existing SDL2/OpenGL 2D/3D, textures, fonts, UI and ECS APIs remain available

NEXUS now has a native windowed runtime for real interactive programs and games. The generated executable loads SDL2 dynamically and uses an OpenGL 2.1 compatibility backend for portable 2D/3D rendering.

Working now:

- native window creation and shutdown
- keyboard state: `gfx_key_down("W")`, `gfx_key_down("ESC")`, etc.
- mouse position and buttons
- event polling and event classification
- frame delta time
- native 2D rectangles, circles, lines and built-in text rendering
- BMP texture loading/drawing/unloading
- texture dimensions
- native 3D camera setup
- 3D cubes and grid rendering
- WAV audio loading and playback
- optional VSync
- runtime window title changes
- project templates: `cli`, `app`, `game`, `3d`
- native 2D app example
- native 2D game example
- native 3D example

The runtime deliberately avoids compile-time SDL headers. Generated applications dynamically load SDL2, which keeps the NEXUS compiler itself independent from SDL2 development headers.

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


### v0.6 runtime additions

NEXUS 0.6 expands the native runtime into a small game/application framework:

- optional PNG/JPG loading through SDL2_image with BMP fallback
- optional TrueType font loading/rendering through SDL2_ttf
- optional music playback through SDL2_mixer (WAV sound effects remain in core SDL2 audio)
- 2D camera position/zoom
- sprite-sheet frame drawing
- cursor visibility control
- immediate-mode UI helpers: buttons, panels, labels and progress bars
- lightweight ECS runtime with entities, transform, velocity, size, color, texture, update, draw and AABB collision
- runtime asset/backend capability reporting

Optional Debian runtime packages for these media features are typically:

```bash
sudo apt install libsdl2-image-2.0-0 libsdl2-ttf-2.0-0 libsdl2-mixer-2.0-0
```

The compiler does not link against these packages at build time; the generated application loads the libraries dynamically when the corresponding feature is used.

### Ultimate showcase

```bash
./build/nexus check examples/ultimate_showcase.nx
./build/nexus build examples/ultimate_showcase.nx -o nexus-ultimate
./nexus-ultimate
```

The showcase exercises the v0.6 runtime in one program: ECS entities, movement, collision, UI, camera, sprite-sheet drawing, TTF text, music, mouse input, keyboard input and native window events.

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

## Create a native application

```bash
./build/nexus new myapp --template app
cd myapp
../nexus/build/nexus run
```

The generated application opens a native window and demonstrates mouse input, event polling, drawing and text.

## Create a 2D game

```bash
./build/nexus new mygame --template game
cd mygame
../nexus/build/nexus build
../nexus/build/nexus run
```

The generated game demonstrates:

- WASD movement
- mouse input
- real frame timing
- native 2D rendering
- collision-ready coordinates
- text HUD
- event loop

## Create a 3D program

```bash
./build/nexus new scene --template 3d
cd scene
../nexus/build/nexus run
```

The generated scene demonstrates an OpenGL 3D camera, grid and cubes.

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

### Native graphics API

```nx
let ok = gfx_init(1280, 720, "My Game")
while !gfx_should_close() {
    gfx_poll()
    let dt = gfx_dt()

    gfx_begin()
    gfx_clear(0.02, 0.03, 0.06, 1.0)
    gfx_rect(20.0, 20.0, 200.0, 80.0, 0.2, 0.6, 1.0, 1.0)
    gfx_circle(320.0, 200.0, 40.0, 1.0, 0.2, 0.2, 1.0)
    gfx_text(40.0, 40.0, 2.0, "NEXUS", 1.0, 1.0, 1.0, 1.0)
    gfx_end()
}
gfx_shutdown()
```

Events:

```nx
gfx_poll()
if gfx_event_is("quit") { ... }
if gfx_event_is("key_down") { ... }
if gfx_event_is("mouse_down") { ... }
let mx = gfx_mouse_x()
let my = gfx_mouse_y()
```

3D:

```nx
gfx3d_begin(70.0, 0.1, 1000.0, 0.0, 2.0, 8.0, 15.0, 0.0, 0.0)
gfx3d_grid(12, 1.0, 0.15, 0.2, 0.3, 1.0)
gfx3d_cube(0.0, 1.0, 0.0, 1.5, 1.5, 1.5, 0.2, 0.8, 1.0, 1.0)
gfx3d_end()
```

Assets:

```nx
let tex = gfx_texture_load("assets/player.bmp")
gfx_texture_draw(tex, 100.0, 100.0, 128.0, 128.0, 1.0, 1.0, 1.0, 1.0)
let sound = gfx_sound_load("assets/hit.wav")
gfx_sound_play(sound, false)
```

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
