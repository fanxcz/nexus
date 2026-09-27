# Nexus language v0.1

## Variables

```nx
let x = 42
let name = "Nexus"
let mut counter = 0
```

## Functions

```nx
fn add(a: i64, b: i64) -> i64 {
    return a + b
}
```

## Control flow

```nx
if x > 10 {
    print(x)
} else {
    print(0)
}
```

```nx
while x < 10 {
    print(x)
}
```

## Arrays

Fixed-size arrays use `[value, value, ...]` literals and can be declared explicitly as `[i64; 3]`. Safe indexing performs a runtime bounds check for non-constant indices.

## Enums and match

Unit enums are declared with `enum Color { Red, Green, Blue }` and variants are written as `Color::Green`. `match` uses `=>` arms and supports `_` as a wildcard.

## Conditional chains

`else if` is supported and is lowered through the same LLVM branch machinery as regular `if`.
