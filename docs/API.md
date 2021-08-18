# API

## Modules

* `tofu.core`
  - `Class`
    - `[f] define(model)`
    - `[f] implement(proto, model)`
    - `[f] dump(table, spaces)`
  - `Math`
    - `[c] SINCOS_PERIOD`
    - `[c] EPSILON`
    - `[f] lerp(a, b, r)`
    - `[f] invlerp(lua_State *L)`
    - `[f] clamp(lua_State *L)`
    - `[f] step(lua_State *L)`
    - `[f] smoothstep(lua_State *L)`
    - `[f] smootherstep(lua_State *L)`
    - `[f] sign(lua_State *L)`
    - `[f] signum(lua_State *L)`
    - `[f] sincos(lua_State *L)`
    - `[f] angle_to_rotation(lua_State *L)`
    - `[f] rotation_to_angle(lua_State *L)`
    - `[f] wave(lua_State *L)`
    - `[f] tweener(lua_State *L)`
  - `System`
    - `[f] args()`
    - `[f] version()`
    - `[f] time()`
    - `[f] fps()`
    - `[f] is_active()`
    - `[f] quit()`
    - `[f] info(...)`
    - `[f] warning(...)`
    - `[f] error(...)`
    - `[f] fatal(...)`
* `tofu.events`
  - `Input`
    - `[f] is_down(id) -> boolean`
    - `[f] is_up(id) -> boolean`
    - `[f] is_pressed(id) -> boolean`
    - `[f] is_released(id) -> boolean`
    - `[f] auto_repeat(id)`
    - `[f] auto_repeat(id, period)`
    - `[f] cursor() -> x, y`
    - `[f] cursor(x, y)`
    - `[f] cursor_area(x, y, width, height)`
    - `[f] stick() -> x, y, angle, magnitude`
    - `[f] triggers() -> left, right`
    - `[f] mode() -> modes`
    - `[f] mode(modes)`
* `tofu.graphics`
  - `Bank`
    - `[f] new(canvas, atlas, cells_file)`
    - `[f] new(canvas. atlas, cell_width, cell_height)`
    - `[m] size(cell_id, scale_x = 1.0, scale_y = 1.0)`
    - `[m] canvas(canvas)`
    - `[m] blit(cell_id, x, y)`
    - `[m] blit(cell_id, x, y, rotation)`
    - `[m] blit(cell_id, x, y, scale_x, scale_y)`
    - `[m] blit(cell_id, x, y, scale_x, scale_y, rotation, anchor_x = 0.5, anchor_y = anchor_x)`
  - `Batch`
    - `[f] new(bank, capacity)`
    - `[m] resize(capacity)`
    - `[m] grow(amounts)`
    - `[m] clear()`
    - `[m] add(cell_id, x, y)`
    - `[m] add(cell_id, x, y, rotation)`
    - `[m] add(cell_id, x, y, scale_x, scale_y)`
    - `[m] add(cell_id, x, y, scale_x, scale_y, rotation, anchor_x = 0.5, anchor_y = anchor_x)`
    - `[m] blit(mode)`
  - `Canvas`
    - `[f] new()`
    - `[f] new(width, height)`
    - `[f] new(file, bf = 0, fg = bg)`
    - `[f] default()`
    - `[m] size()`
    - `[m] center()`
    - `[m] push()`
    - `[m] pop()`
    - `[m] reset()`
    - `[m] background(index)`
    - `[m] color(index)`
    - `[m] shift()`
    - `[m] shift(table)`
    - `[m] shift(from, to)`
    - `[m] transparent()`
    - `[m] transparent(table)`
    - `[m] transparent(index, is_transparent)`
    - `[m] clipping()`
    - `[m] clipping(x, y, width, height)`
    - `[m] clear(color = background)`
    - `[m] point(x, y, color = color)`
    - `[m] hline(x, y, width, color = color)`
    - `[m] vline(x, y, height, color = color)`
    - `[m] line(x0, y0, x1, y1, color = color)`
    - `[m] polyline(vertices, color = color)`
    - `[m] fill(x, y, color = color)`
    - `[m] triangle(mode, x0, y0, x1, y1, x2, y2, color = color)`
    - `[m] rectangle(mode, x, y, width, height, color = color)`
    - `[m] square(mode, x, y, size, color = color)`
    - `[m] circle(mode, cx, cy, radius, color = color)`
    - `[m] peek(x, y)`
    - `[m] poke(x, y)`
    - `[m] process(callback, x, y, canvas = self)`
    - `[m] process(callback, x, y, ox, oy, width, height, canvas = self)`
    - `[m] copy(x, y, canvas = self)`
    - `[m] copy(x, y, ox, oy, width, height, canvas = self)`
  - `Display` *OK*
    - `[f] palette(palette)`
    - `[f] switch(id = 0)`
    - `[f] offset()`
    - `[f] offset(x = 0, y = 0)`
    - `[f] bias()`
    - `[f] bias(amount = 0)`
    - `[f] shift()`
    - `[f] shift(from, to)`
    - `[f] shift(table)`
    - `[f] copperlist(copperlist = nil)`
  - `Palette` *OK*
    - `[f] new()`
    - `[f] new(id)`
    - `[f] new(shades)`
    - `[f] new(colors)`
    - `[f] mix(ar, ag, ab, br, bg, bb, ratio)`
    - `[m] colors() -> colors`
    - `[m] size() -> number of colors`
    - `[m] color_to_index(r, g, b) -> integer`
    - `[m] index_to_color(index) -> r, g, b`
    - `[m] lerp(r, g, b, ratio)`
    - `[m] merge(palette, remove_duplicates = true)`
  - `Copperlist` *OK*
    - `[f] new()`
    - `[m] wait(x, y)`
    - `[m] modulo(value)`
    - `[m] offset(value)`
    - `[m] palette(id)`
    - `[m] color(index, r, g, b)`
    - `[m] bias(amount)`
    - `[m] shift(from, to)`
    - `[m] shift(table)`
    - `[m] gradient(index, markers)`
    - `[m] palette(x, y, palette)`
  - `Font`
  - `XForm` *OK*
    - `[f] new(mode = "repeat")`
    - `[m] offset(h, v)`
    - `[m] matrix(x0, y0)`
    - `[m] matrix(a, b, c, d)`
    - `[m] matrix(a, b, c, d, x0, y0)`
    - `[m] wrap(mode)`
    - `[m] table()`
    - `[m] table(table)`
    - `[m] project(height, angle, elevation)`
    - `[m] warp(height, factor)`
* `tofu.io`
  - `File` *OK*
    - `[f] load(name, mode = "string")`
    - `[f] store(name, data, mode = "string")`
* `tofu.sound`
  - `Speakers`
    - `[f] volume() -> level`
    - `[f] volume(level)`
    - `[f] mix()`
    - `[f] mix(group_id) -> left_to_left, left_to_right, right_to_left, right_to_right`
    - `[f] mix(group_id, left_to_left, left_to_right, right_to_left, right_to_right)`
    - `[f] pan(group_id, value)`
    - `[f] balance(group_id, value)`
    - `[f] gain(group_id) -> level`
    - `[f] gain(group_id, level)`
    - `[f] halt()`
  - `Source`
    - `[f] new(file, type)`
    - `[m] looped() -> is_looped`
    - `[m] looped(is_looped)`
    - `[m] group() -> group_id`
    - `[m] group(group_id)`
    - `[m] mix() -> left_to_left, left_to_right, right_to_left, right_to_right`
    - `[m] mix(left_to_left, left_to_right, right_to_left, right_to_right)`
    - `[m] pan(value)`
    - `[m] pan(left, right)`
    - `[m] balance(value)`
    - `[m] gain() -> level`
    - `[m] gain(level)`
    - `[m] speed() -> ratio`
    - `[m] speed(ratio)`
    - `[m] play()`
    - `[m] resume()`
    - `[m] stop()`
    - `[m] is_playing() -> is_playing`
* `tofu.timers`
  - `Pool`
  - `Timer`
* `tofu.util`
  - `Arrays`
  - `Grid`
  - `Iterators`
  - `Vector`