# Tofu Engine

Welcome!

Guess what? Yup, that's yet another game engine/framework.

## Pros

* Carefully crafted C99 code.
* Self-contained, no additional runtime modules/libraries required (system-wide libraries excluded).
* Multi-platform through cross-compilation (Windows, Linux and Raspberry-Pi... no MacOS won't be supported, for the moment).

## Dependecies

* [Glad](https://glad.dav1d.de/)
* [GLFW](https://www.glfw.org/) v3.3.2
* [Lua](https://lua.org/) v5.3.5
* [miniaudio](https://github.com/dr-soft/miniaudio) v0.10.5
* [dr_libs](https://github.com/mackron/dr_libs) v0.12.12, v0.6.9, v0.12.3
* [spleen](https://github.com/fcambus/spleen) fonts
* [stb](https://github.com/nothings/stb) libraries

## Features

* [x] Fully script-based, using Lua.
* [x] Straight multimedia support, no intermediate third-party libraries (OpenGL 2.1 required).
* [x] Windowed/fullscreen display with automatic scaling.
* [x] Internal software renderer.
* [x] Fixed- and variable-size *BOBs*' blitting with rotation/scaling/flipping.
* [x] Palette based graphics with up to 256 colors.
* [x] Predefined library of 8/16/32/64 colors palettes.
* [x] Automatic nearest-color palette indexing of images.
* [x] Per-color re-indexing (*shifting*) and transparency, affecting drawing operations (global, too?).
* [x] Audio support w/ real time sound streaming on a separate thread.
* [x] On-the-fly audio mixing w/ per voice looping/panning/gain/speed control.
* [x] Out-of-the-box timers support.
* [x] Customizable application icon.
* [x] Support for *archived games*, via custom "packed" format (w/ optional encryption).
* [x] Game-controller support (w/ D-PAD and mouse emulation) w/ keyboard/mouse fallback if not available.
* [x] Tiled-map support w/ camera support (zoom and scrolling).
* [x] Screen shaking.
* [x] Out-of-the-box 'tweening functions support (optimized [Penner's](http://robertpenner.com/easing/) set).
* [x] Detailed logging facility (w/ logging level throttle).
* [x] Crash screen (debug build).

## Desiderata

* [ ] **Bit** **Bl**ock **T**ransfer operations when drawing (also, [stencil](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) support, see [this](https://open.gl/depthstencils)).
* [ ] Animation support w/ frameset DSL (i.e. compiling a string where each token can be a single frame, a range or a "keep-current-frame for some time" command). Each frameset can have its one update period, and will be most likely based upon a timer.
* [ ] Out-of-the-box palette switching (with tweening) features.
* [ ] Game state and display transitions (at which level? Engine or script?).
* [ ] Library of noise functions ([cellular](https://thebookofshaders.com/12/), Perlin, etc...).
* [ ] Library of "retro-feel" shaders.
* [ ] Multiple players support.
* [ ] Define some fixed resolutions (see [this](https://pacoup.com/2011/06/12/list-of-true-169-resolutions/))?
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
