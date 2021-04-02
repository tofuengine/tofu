# Introduction

Welcome to `Tofu Engine`!

Guess what? Yup, that's yet another game engine/framework.

## Highlights

* Carefully crafted C99 code.
* Self-contained, no additional runtime modules/libraries required (system-wide libraries excluded).
* Multi-platform support through cross-compilation (Windows, Linux and [Raspberry-Pi](https://www.raspberrypi.org/) -- macOS currently not supported).

## Features

* [x] Fully script-based, using [Lua](https://www.lua.org/).
* [x] Straight multimedia support, no intermediate third-party libraries (OpenGL 2.1 required).
* [x] Windowed/fullscreen display with best-fit integer automatic scaling.
* [x] Internal software renderer. OpenGL is used only to present the framebuffer to the user (and apply post-process effects).
* [x] Fixed- and variable-size *Blitter OBjects* drawing with rotation/scaling/flipping.
* [x] Sprite batching for optimized (ehm) batch drawing.
* [x] Palette based graphics with up to 256 colors.
* [x] Multiple (i.e. banked) palette support w/ up to 8 palettes, and color bias during VRAM transfer.
* [x] Predefined library of 8/16/32/64 colors palettes.
* [x] Automatic nearest-matching-color palette indexing of [RGBA8888](https://en.wikipedia.org/wiki/RGBA_color_model) images.
* [x] Per-color re-indexing (*shifting*) and transparency, affecting drawing operations (both per-draw and during VRAM transfer).
* [x] Multiple canvas, with drawing state stack support.
* [x] SNES' Mode7-like transforms, with scanline based ([HDMA](https://wiki.superfamicom.org/grog's-guide-to-dma-and-hdma-on-the-snes)) changes.
* [x] Amiga's Copper-like programs, with pixel-wise resolution.
* [x] Post-effect display-wise fragment shaders.
* [x] Library of "retro-feel" post-effects (LCD, CRT, etc...).
* [x] Audio support w/ real time sound streaming on a separate thread.
* [x] On-the-fly audio mixing w/ per voice looping/panning/balance/gain/speed control.
* [x] Static and streamed audio data playback (FLAC format).
* [x] Module playback support (MOD, S3M, XM, and IT).
* [x] Out-of-the-box timers support.
* [x] Customizable application icon.
* [x] Support for *archived games*, via custom "packed" format (w/ optional encryption). Multiple archives are supported, with root folder override.
* [x] Resource manager w/ caching I/O and single instance object loading/reuse.
* [x] Game-controller support (w/ D-PAD and mouse emulation) w/ keyboard/mouse fallback if not available.
* [x] Screen capture and recording.
* [x] Framebuffer offsetting (e.g. for screen-shaking effect).
* [x] Out-of-the-box 'tweening functions support (optimized [Penner's](http://robertpenner.com/easing/) set).
* [x] Detailed logging facility (w/ selectable severity level).
* [x] Crash screen (debug build).
* [x] Game window focus detection (for game-pause).
* [x] Real-time performance statistics (FPS and frame times).
* [x] User-dependent I/O functions to load/store game data.
* [x] Configuration override through command-line arguments.

## Dependencies

* [dr_libs](https://github.com/mackron/dr_libs) v0.12.28, v0.6.27, v0.12.19
* [Glad](https://glad.dav1d.de/)
* [gif-h](https://github.com/charlietangora/gif-h)
* [GLFW](https://www.glfw.org/) v3.3.3
* [libxmp](http://xmp.sourceforge.net/) v4.5.0
* [Lua](https://lua.org/) v5.4.3
* [miniaudio](https://github.com/dr-soft/miniaudio) v0.10.32
* [spleen](https://github.com/fcambus/spleen) v1.8.1
* [stb](https://github.com/nothings/stb) libraries

## Inspirations

**#tofuengine** is an original software, result of the experience gained from ~30 years in programming on a broad range of platforms (some concept even stems back to the [Amiga](https://en.wikipedia.org/wiki/Amiga) and the [SNES](https://en.wikipedia.org/wiki/Super_Nintendo_Entertainment_System)). However, it has also been influenced by modern similar/other game-engines in one way or another. Here's a brief list.

* [Love2D](https://love2d.org/)
* [Pico-8](https://www.lexaloffle.com/pico-8.php)
* [picolove](https://github.com/picolove/picolove/)
* [Defold](https://defold.com/)
* [raylib](https://www.raylib.com/)

# Compiling

In order to compile `Tofu Engine`, a Linux machine in required (either physical or virtual). A Debian-based distribution is suggested. One can issue the following commands to install all the required dependencies:

```bash
sudo apt install git
sudo apt install build-essential
sudo apt install mingw-w64
sudo apt install libx11-dev mesa-common-dev libgles2-mesa-dev
sudo apt install lua5.3 liblua5.3-dev luarocks
sudo luarocks install argparse
sudo luarocks install luafilesystem
sudo luarocks install luacheck
sudo luarocks install luacheck
sudo luarocks install luazen
sudo luarocks install lua-struct
sudo apt install imagemagick
```

Of course, `git` should also be installed to clone the repository.

```bash
sudo apt install git
```

Proceed in creating a local clone of the repository with the command

```bash
git clone https://github.com/tofuengine/tofu.git
```

into a suitable work folder. Change directory into `tofu` folder you've just created and use `make` to build the executable. You can use the following command-line parameters to control the build process:

* `BUILD`, can be either `debug` or `release` with the usual meaning. If not specified, the build is assumed in **debug** mode.
* `PLATFORM`, can be either `linux`, `windows`, or `rpi`. If not specified, the build is assumed for **Linux** platform. Please not that while the Windows build is generated on Linux using cross-compiling, the *Raspberry-Pi* build can be obtained only on a proper Raspberry-Pi board computer.
* `ARCHITECTURE`, can be either `x64` or `x32`. Please note that this is used only for the **Windows** build as the Linux one is 64-bit only, and the Raspberry-Pi 32-bit only.

## Sample projects

Along with the game-engine source, there's a bunch of (basic) demo projects. They are located in the `demos` sub-folder and can be launched using `make`, passing the name of the project as a target (e.g. `make bunnymark`).

# Addenda

## Desiderata

* [ ] Multi-threaded parallel rendering (w/ double/triple buffering).
* [ ] Masking functions for both drawing primitives and blits.
* [ ] **Bit** **Bl**ock **T**ransfer operations when drawing (also, [stencil](https://learnopengl.com/Advanced-OpenGL/Stencil-testing) support, see [this](https://open.gl/depthstencils)).
* [ ] Define some fixed resolutions (see [this](https://pacoup.com/2011/06/12/list-of-true-169-resolutions/))?
* [ ] Tiled-map support w/ camera support (zoom and scrolling).
* [ ] Animation support w/ frameset DSL (i.e. compiling a string where each token can be a single frame, a range or a "keep-current-frame for some time" command). Each frameset can have its one update period, and will be most likely based upon a timer.
* [ ] Custom "raw" graphics and sound formats, with on-the-fly LZ4 (stream?) compression.
* [ ] Game state and display transitions (at which level? Engine or script?).
* [ ] Library of noise functions ([cellular](https://thebookofshaders.com/12/), Perlin, etc...).
* [ ] Multiple players support.
* [ ] Use a custom memory-management allocator.
* [ ] Memory usage profiling.
* [ ] Switch to [Vulkan API](https://www.khronos.org/vulkan/) (through [GLFW](https://www.glfw.org/)).
* [ ] game time management, in system class (speed, up down pause)
* [ ] async load with callback
* [ ] memory usage info in system class
* [ ] both shoulder and trigger axes are analogue?
* [ ] rumble?
* [ ] analogues low pass filter (page 591) or moving average?
* [ ] buttons states with XOR (page 594)
* [ ] chords and gestures detection
* [ ] collisions

## Profiling

```bash
make bunnymark BUILD=profile
prof ./tofu  gmon.out > analysys.txt
prof ./tofu  gmon.out | ./extras/gprof2dot.py | dot -Tpng -o analysys.png
```
