# Nexus Paint

Nexus Paint is a native raster editor built with the NEXUS runtime.

## Create

```bash
nexus new NexusPaint --template paint
cd NexusPaint
nexus build --release
nexus run
```

## Tools

- Pencil
- Eraser
- Line
- Rectangle
- Flood fill
- Color palette
- Brush size
- Undo / redo (32 checkpoints)

## Files

Paint saves and loads `nexus_paint.bmp` as an uncompressed 32-bit BMP.

## Runtime API

The editor uses the native CPU-backed canvas API:

```text
gfx_canvas_create
gfx_canvas_brush
gfx_canvas_line
gfx_canvas_rect
gfx_canvas_fill
gfx_canvas_draw
gfx_canvas_save_bmp
gfx_canvas_load_bmp
gfx_canvas_checkpoint
gfx_canvas_undo
gfx_canvas_redo
```
