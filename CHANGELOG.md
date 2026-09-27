# Changelog

## 1.1.0 - Graphics backend rewrite

- SDL2 2D renderer is now the default for windowed applications and Nexus Paint.
- 2D rendering no longer depends on an OpenGL compatibility profile.
- Canvas creation/destruction is backend-safe.
- Resize, maximize and fullscreen APIs are exposed to Nexus.
- Virtual UI coordinates remain stable across window sizes.
- `gfx_backend()` reports the active renderer.
- Runtime graphics code is organized into explicit 2D, 3D, canvas, input and window sections.
- OpenGL is activated only when a 3D frame is requested.
- `ctest` and native graphics smoke tests are part of the release validation.
- `usize`/`isize` aliases map to the platform integer width supported by the current backend.
- Added low-level `mem_set` and `mem_copy` runtime operations alongside `mem_alloc`/`mem_free`.

