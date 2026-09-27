# Architecture

```text
Nexus source
   |
 Lexer
   |
 Parser
   |
 AST
   |
 Name / type analysis
   |
 Target-independent Nexus representation
   |
 LLVM IR generator
   |
 LLVM target triple
   |
 Clang/LLVM + platform toolchain
   |
 Native executable / object / WASM
```

The compiler deliberately separates:

- host platform
- target platform
- target triple
- runtime
- linker/toolchain

The frontend does not contain Linux-specific logic. Platform-specific behavior belongs in the runtime/toolchain layer.

## v0.2 compiler modules

```text
compiler/
├── lexer/
├── parser/
├── ast/
├── semantic/
├── codegen/
└── driver/
```

The next architectural step is to add explicit HIR/MIR layers between semantic analysis and LLVM so increasingly advanced language features do not leak backend concerns into the parser.
