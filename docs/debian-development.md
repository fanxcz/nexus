# Debian 13 development

Nexus is developed and tested first on Debian 13.

Install the common build toolchain:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git clang lld
```

Verify:

```bash
c++ --version
cmake --version
ninja --version
clang --version
ld.lld --version
```

The current backend emits textual LLVM IR and invokes Clang/LLVM for native lowering and linking. This keeps the project buildable even when distro LLVM C++ development headers are unavailable.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Install

```bash
cmake --install build --prefix ~/.local
```

The runtime is installed next to the compiler's data files, so `nexus run` can work from another directory.

## Native graphics runtime

For running generated windowed NEXUS applications on Debian, install the SDL2 runtime and OpenGL runtime:

```bash
sudo apt install libsdl2-2.0-0 libgl1
```

The compiler does not require SDL2 development headers; generated binaries dynamically load SDL2 at runtime. Debian 13/trixie provides `libsdl2-2.0-0`.
