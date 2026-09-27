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
