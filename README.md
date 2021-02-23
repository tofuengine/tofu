# Tofu Engine

Welcome!

Guess what? Yup, that's yet another game engine/framework.

## Highlights

* Carefully crafted C99 code.
* Self-contained, no additional runtime modules/libraries required (system-wide libraries excluded).
* Multi-platform support through cross-compilation (Windows, Linux and [Raspberry-Pi](https://www.raspberrypi.org/) -- macOS currently not supported).

## Features

* [x] Fully script-based, using Lua.
* [x] Straight multimedia support, no intermediate third-party libraries (OpenGL 2.1 required).
* [x] Windowed/fullscreen display with best-fit integer automatic scaling.
* [x] Internal software renderer.
* [x] Fixed- and variable-size *Blitter OBjects* drawing with rotation/scaling/flipping.
* [x] Sprite batching for optimized (ehm) batch drawing.
* [x] Palette based graphics with up to 256 colors.
* [x] Predefined library of 8/16/32/64 colors palettes.
* [x] Automatic nearest-color palette indexing of images.
* [x] Per-color re-indexing (*shifting*) and transparency, affecting drawing operations (global, too?).
* [x] SNES' Mode7-like transforms, with scanline based (HDMA) changes.
* [x] Audio support w/ real time sound streaming on a separate thread.
* [x] On-the-fly audio mixing w/ per voice looping/panning/balance/gain/speed control.
* [x] Static and streamed audio data playback (FLAC format).
* [x] Module playback support (MOD, S3M, XM, and IT).
* [x] Out-of-the-box timers support.
* [x] Customizable application icon.
* [x] Support for *archived games*, via custom "packed" format (w/ optional encryption).
* [x] Game-controller support (w/ D-PAD and mouse emulation) w/ keyboard/mouse fallback if not available.
* [x] Screen capture and recording.
* [x] Framebuffer offsetting (e.g. for screen-shaking effect).
* [x] Out-of-the-box 'tweening functions support (optimized [Penner's](http://robertpenner.com/easing/) set).
* [x] Detailed logging facility (w/ logging selectable level).
* [x] Crash screen (debug build).
* [x] Game window focus detection (for game-pause).
* [x] Real-time performance statistics (FPS and frame times).

## Desiderata

* [ ] Masking functions for both drawing primitives and blits.
* [ ] **Bit** **Bl**ock **T**ransfer operations when drawing (also, [stencil](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) support, see [this](https://open.gl/depthstencils)).
* [ ] Define some fixed resolutions (see [this](https://pacoup.com/2011/06/12/list-of-true-169-resolutions/))?
* [ ] Tiled-map support w/ camera support (zoom and scrolling).
* [ ] Animation support w/ frameset DSL (i.e. compiling a string where each token can be a single frame, a range or a "keep-current-frame for some time" command). Each frameset can have its one update period, and will be most likely based upon a timer.
* [ ] Custom "raw" graphics and sound formats, with on-the-fly LZ4 (stream?) compression.
* [ ] Out-of-the-box palette switching (with tweening) features.
* [ ] Game state and display transitions (at which level? Engine or script?).
* [ ] Library of noise functions ([cellular](https://thebookofshaders.com/12/), Perlin, etc...).
* [ ] Library of "retro-feel" shaders.
* [ ] Multiple players support.
* [ ] Use a custom memory-management allocator.
* [ ] Memory usage profiling.
* [ ] Switch to [Vulkan API](https://www.khronos.org/vulkan/) (through [GLFW](https://www.glfw.org/)).
* [ ] game time management, in system class (speed, up down pause)
* [ ] resource manager, with single object loading
* [ ] async load with callback
* [ ] memory usage info in system class
* [ ] both shoulder and trigger axes are analogue?
* [ ] rumble?
* [ ] analogues low pass filter (page 591) or moving average?
* [ ] buttons states with XOR (page 594)
* [ ] chords and gestures detection
* [ ] collisions

## Dependencies

* [dr_libs](https://github.com/mackron/dr_libs) v0.12.26, v0.6.25, v0.12.17
* [Glad](https://glad.dav1d.de/)
* [gif-h](https://github.com/charlietangora/gif-h)
* [GLFW](https://www.glfw.org/) v3.3.2
* [libxmp](http://xmp.sourceforge.net/) v4.5.0
* [Lua](https://lua.org/) v5.4.2
* [miniaudio](https://github.com/dr-soft/miniaudio) v0.10.32
* [spleen](https://github.com/fcambus/spleen) v1.8.1
* [stb](https://github.com/nothings/stb) libraries

## Inspirations

**#tofuengine** is an original software, born from a ~30 years long experience in programming. However, it has been influenced during its development by similar/other game-engines in one way or another. Here's a brief list.

* [Love2D](https://love2d.org/)
* [Pico-8](https://www.lexaloffle.com/pico-8.php)
* [picolove](https://github.com/picolove/picolove/)
* [Defold](https://defold.com/)
* [raylib](https://www.raylib.com/)

# Profiling

```bash
make bunnymark BUILD=profile
prof ./tofu  gmon.out > analysys.txt
prof ./tofu  gmon.out | ./extras/gprof2dot.py | dot -Tpng -o analysys.png
```
