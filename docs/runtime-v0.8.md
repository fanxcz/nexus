# NEXUS 0.8 Runtime API

NEXUS 0.8 adds higher-level application/game building blocks on top of the SDL2/OpenGL runtime.

## Sprite animation

```nx
let tex = gfx_texture_load("assets/sheet.bmp")
let anim = gfx_anim_create(tex, 32, 32, 4, 8, 12.0, true)
gfx_anim_update(anim, gfx_dt())
gfx_anim_draw(anim, 100.0, 100.0, 128.0, 128.0, 1.0, 1.0, 1.0, 1.0)
```

## Collision

```nx
if physics_aabb(ax, ay, aw, ah, bx, by, bw, bh) {
    print("AABB hit")
}

if physics_circle(ax, ay, ar, bx, by, br) {
    print("circle hit")
}
```

## Scenes

Scenes snapshot ECS entities and can be saved as deterministic text files.

```nx
let scene = scene_create()
let e = ecs_create()
ecs_set_position(e, 10.0, 20.0)
scene_add(scene, e)
scene_save(scene, "save.nxs")
let loaded = scene_load("save.nxs")
scene_destroy(loaded)
scene_destroy(scene)
```

## GUI

```nx
let enabled = ui_checkbox(40.0, 80.0, "Enable effects", true)
let volume = ui_slider(40.0, 130.0, 240.0, 24.0, 0.75)
```

The runtime remains usable without the optional SDL2_image, SDL2_ttf and SDL2_mixer libraries. Basic graphics and core PCM WAV audio are provided by the SDL2 core runtime.
