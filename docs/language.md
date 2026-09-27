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


## Interactive runtime

Nexus 0.4 provides built-in runtime calls for interactive programs.

```nx
let name = input("Name: ")
let age = input_i64("Age: ")
print("Hello, " + name)
print(str_i64(age + 1))
```

Persistence and environment:

```nx
file_write("save.txt", "hello")
let data = file_read("save.txt")
let exists = file_exists("save.txt")
let home = env("HOME")
```

## Terminal games

A terminal game can use a real update loop:

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

The current game framework is terminal-native. A separate graphical backend can be added without changing the core language syntax.

## Project templates

```bash
nexus new mycli --template cli
nexus new myapp --template app
nexus new mygame --template game
```
