# NEXUS 1.0.0

NEXUS is a native, cross-platform programming language and application/game runtime. The compiler is developed on Debian 13 with C++20 and emits LLVM IR/native binaries through Clang/LLVM.

## What is working

- Native compiler pipeline: lexer -> parser -> semantic checking -> LLVM IR -> native binary.
- Project workflow: `new`, `init`, `build`, `run`, `check`, `clean`, `info`, `assets`, `doctor`.
- Tooling: `fmt`, `lint`, `doc`, `bench`, `editor`, `lsp`.
- Local package workflow: `nexus pkg init/add/remove/list/update/build/publish`.
- Runtime: memory allocation, files, environment, input, HTTP GET facade, lightweight JSON field extraction, dynamic SQLite runtime loading, SHA-256 helper, SDL2/OpenGL 2D/3D, audio, UI, ECS, scenes, particles and physics.
- Targets: Linux x86_64 first-class; target triples are explicit and cross-compilation fails clearly when a local toolchain/sysroot is missing.

## Create a project

```bash
nexus new MyGame --template game
cd MyGame
nexus build
nexus run
```

## Standard modules

```nx
import std.io
import std.fs
import std.net
import std.json
import std.db
import std.memory
import std.crypto
```

## Tooling

```bash
nexus fmt
nexus lint
nexus doc
nexus bench --runs 5
nexus test
nexus editor
nexus lsp
```

## Package workflow

Local packages can be vendored into `vendor/` and locked in `nexus.lock`:

```bash
nexus pkg init
nexus pkg add ../my-library
nexus pkg list
nexus pkg build
nexus pkg publish
```

## Important limits in 1.0.0

The runtime/tooling foundation is production-oriented, but some advanced language features remain experimental or are still being designed: full generic monomorphization, trait dispatch, closures/async lowering, a full visual scene editor, and a complete JSON AST/HTTP server stack. They are isolated behind stable interfaces so they can be added without redesigning the project format.
