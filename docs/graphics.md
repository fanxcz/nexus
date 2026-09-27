# NEXUS Native Graphics

NEXUS 0.5 adds a native graphics runtime built around SDL2 + OpenGL. The compiler itself does not depend on SDL headers; generated applications load the SDL2 runtime dynamically.

## Window and frame loop

```nx
let ok = gfx_init(1280, 720, "My Game")
while !gfx_should_close() {
    gfx_poll()
    gfx_begin()
    gfx_clear(0.02, 0.03, 0.06, 1.0)
    gfx_end()
}
gfx_shutdown()
```

## Input and events

```nx
gfx_poll()
if gfx_key_down("W") { ... }
if gfx_mouse_down(1) { ... }
let mx = gfx_mouse_x()
let my = gfx_mouse_y()
let event = gfx_event_type()
```

Supported named keys include `W`, `A`, `S`, `D`, `SPACE`, `ENTER`, `ESC`, `LEFT`, `RIGHT`, `UP`, `DOWN`, `TAB`, `SHIFT`, and `CTRL`.

## 2D

```nx
gfx_begin()
gfx_rect(20.0, 20.0, 100.0, 50.0, 0.2, 0.7, 1.0, 1.0)
gfx_circle(200.0, 120.0, 30.0, 1.0, 0.2, 0.3, 1.0)
gfx_line(0.0, 0.0, 500.0, 300.0, 3.0, 1.0, 1.0, 1.0, 1.0)
gfx_text(30.0, 30.0, 2.0, "HELLO NEXUS", 1.0, 1.0, 1.0, 1.0)
gfx_end()
```

## Textures

`gfx_texture_load()` currently accepts BMP images through SDL2. PNG/JPEG loading is reserved for a future image codec layer.

```nx
let tex = gfx_texture_load("assets/player.bmp")
gfx_texture_draw(tex, 100.0, 100.0, 128.0, 128.0, 1.0, 1.0, 1.0, 1.0)
gfx_texture_unload(tex)
```

## 3D

The initial 3D backend targets the OpenGL 2.1 compatibility profile so the same API can be kept simple across Linux/Windows/macOS.

```nx
gfx3d_begin(70.0, 0.1, 1000.0, 0.0, 2.0, 8.0, 20.0, 0.0, 0.0)
gfx3d_grid(10, 1.0, 0.2, 0.3, 0.4, 1.0)
gfx3d_cube(0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.2, 0.8, 1.0, 1.0)
gfx3d_end()
```

## Sound

The runtime can load WAV data through SDL2 core audio.

```nx
let sound = gfx_sound_load("assets/hit.wav")
gfx_sound_play(sound, false)
```

The runtime dynamically loads SDL2, so the generated application requires an SDL2 runtime on the target machine.
