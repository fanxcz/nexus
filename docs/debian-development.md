# Debian 13 development

Nexus is developed and tested first on Debian 13.

Install the common build toolchain:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git clang lld
```

Check:

```bash
c++ --version
cmake --version
ninja --version
clang --version
ld.lld --version
```

The v0.1 backend emits textual LLVM IR and asks Clang to lower/link it. This keeps the source tree buildable even when distro LLVM development headers are unavailable. A future LLVM-C++ backend can be selected when LLVM development packages are installed.
