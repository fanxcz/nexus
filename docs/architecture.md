# Architecture

```text
Nexus source
   |
 Lexer
   |
 Parser -> AST
   |
 Semantic analysis
   |
 Target-independent compiler data
   |
 LLVM IR generator
   |
 Clang/LLVM target backend
   |
 Native executable
```

The compiler is designed around host/target separation. Platform-specific runtime and linker logic must stay outside the language frontend.
