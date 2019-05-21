# Tofu Engine

Welcome!

Guess what? Yup, that's another game engine/framework.

## Pros

* C99 code only.
* Self-contained, no external modules/libraries required.
* Cross-compiling multi-platform (hopefully).

## Cons

* **Wren** is not as popular as others (e.g. **Lua** or **JavaScript**), which translates into less *ready-to-use* code. However, embedding is much more seamless than with **Lua**.

## Uses

* [GLFW](https://www.glfw.org/) v3.3
* [Glad](https://glad.dav1d.de/)
* Sean Barret's [stb](https://github.com/nothings/stb) libraries
* [Wren](https://wren.io/) v0.1.0
* [jsmn](https://zserge.com/jsmn.html/) v1.0.0

## Features

* [x] Palette based graphics (through shader) with up to 64 colors.
* [x] Automatic nearest-color palette indexing of images.
* [x] Predefined library of 8/16/32/64 colors palettes.
* [ ] Tiled-map support w/ camera support (shader-level zoom and scrolling?).
* [x] Out-of-the-box timers support.
* [ ] Out-of-the-box easing functions.
* [ ] Animation support w/ frameset DSL (i.e. compiling a string where each token can be a single frame, a range or a "keep-current-frame for some time" command). Each frameset can have its one update period, and will be most likely based upon a timer.
* [ ] Out-of-the-box palette switching features.
* [ ] Game state and display transitions (at which level? Engine or script?).
* [ ] **Bit** **Bl**ock **T**ransfer operations when drawing.
* [ ] Library of "retro-feel" shaders.
* [ ] Camera/screen shaking by using a post-processing shader.
* [ ] Engine splash screen (during which resources are loaded).
* [ ] Hot-reload of selected resources (fonts, banks, maps, shaders, sounds).
* [ ] Digital/analogue game-controller support.
* [ ] Support for TARed/ZIPed games ([rxi/microtar](https://github.com/rxi/microtar), [kuba--/zip](https://github.com/kuba--/zip)).

## TODO

* [ ] Define some fixed resolutions (see [this](https://pacoup.com/2011/06/12/list-of-true-169-resolutions/))?
* [ ] Use a *smarter* string library (implement or use [utstring](http://troydhanson.github.io/uthash/utstring.html)).
* [ ] Use a custom memory-management allocator.
* [ ] Switch to [Vulkan API](https://www.khronos.org/vulkan/) (through [GLFW](https://www.glfw.org/)).
* [ ] Change the API to be event-based (with explicit registration).

## Inspirations

* [LOVE2D](https://love2d.org/)
* [raylib](https://www.raylib.com/)
* [DOME Engine](https://github.com/avivbeeri/dome/)

## License

Copyright (c) 2019 Marco Lizza (marco.lizza@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
