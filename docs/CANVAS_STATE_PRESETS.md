# Canvas state presets

### Context

The canvas drawing state has grown beyond a small collection of scalar values. It now includes clipping, palette bank selection, palette shifting, transparency, and the palette-state lookup tables used by the blitters. This makes the state valuable as a reusable drawing profile, not only as something to save temporarily around a scoped rendering operation.

The existing `push()` and `pop()` model is still useful for scoped changes. It answers the question, "how do I temporarily modify the current drawing state and then return to the previous one?" A preset model answers a different question: "how do I prepare a named drawing state and recall it whenever I need it?" These two behaviors should stay distinct.

### Palette-state LUTs

The palette-state map was changed from a single 8-bit lookup table plus a separate bank mask into bank-specific lookup tables. Each bank has its own LUT row that stores the precomputed destination pixel and the transparency flag. The active bank selects the LUT row once before entering the hot drawing loop.

This keeps bank switching cheap. A single-map `uint16_t` version removed the per-pixel bank `or`, but made `bank()` rebake the LUT. That was the wrong tradeoff because the old per-pixel `or` was cheap, while bank switching can happen often and should remain `O(1)`.

The current 2D LUT shape is the better version of the precomputed-bank approach. The full palette state is larger, but the hot loops touch only the active bank row. With two banks, each active row is still small enough to fit comfortably in cache.

### Synced and bank-local modes

`TOFU_GRAPHICS_PALETTE_BANK_LUT_SYNC` keeps palette shifting and transparency changes synchronized across every bank LUT. This preserves the old semantic model where `shift()` and `transparent()` describe the canvas drawing state independently from the active palette bank.

Disabling that macro makes shifting and transparency bank-local. This can be interesting because `bank()` then selects not only a color bank, but also a remap/transparency profile. That can support instant flashes, silhouettes, masks, or alternate sprite appearances without touching image data. The tradeoff is that the meaning of `shift()` and `transparent()` changes: they only modify the currently selected bank LUT. Because that behavior is powerful but less obvious, synced mode should remain the default.

### State Controller

Rather than adding more top-level methods to `Canvas`, the preferred API direction is to move drawing-state operations under a state controller returned by `canvas:state()`. This gives state management a real namespace and keeps the canvas object focused on drawing operations.

The state controller should be a live controller for the canvas context, not a snapshot. Calling `canvas:state()` should return an object that manipulates the canvas' current `GL_Context_t` state. A snapshot is created only when `capture(name)` is called.

With this approach, the state-related methods can move from `Canvas` to the state controller. That includes `reset`, `push`, `pop`, `clipping`, `shift`, `transparent`, and `bank`. Since API compatibility is not a requirement, keeping top-level aliases is not necessary and would weaken the cleanup.

### Preset Semantics

`capture(name)` copies the current drawing state into a named preset slot. `recall(name)` replaces the current drawing state with the captured copy. `forget(name)` removes one captured preset, while `forget()` may clear all presets if that behavior is desired.

`recall(name)` should not remove the captured state. Presets are meant to be reusable. This is the main reason not to overload `push()` and `pop()` for this feature: a stack pop either removes the saved state, making it less useful as a preset, or keeps it, breaking the expected stack semantics.

The preset store should probably live in `GL_Context_t`, close to `current` and `stack`, because presets are copies of `GL_State_t`. The Lua-facing state controller can be a small proxy around the canvas context. If the proxy is cached per canvas, repeated `canvas:state()` calls remain cheap and stable.

### Example

```lua
local state = canvas:state()

state:reset()

state:push()
state:clipping(0, 0, 128, 128)
state:bank(1)
state:shift({
  [1] = 7,
  [2] = 7,
  [3] = 7,
})
state:transparent(0, true)
state:capture("flash")
state:pop()

state:push()
state:bank(0)
state:transparent(0, true)
state:capture("normal")
state:pop()

state:recall("normal")
canvas:sprite(x, y, bank, cell)

state:recall("flash")
canvas:sprite(x, y, bank, cell)

state:forget("flash")
```

In this example, `push()` and `pop()` are used for temporary setup while preparing presets. `capture()` stores the configured state under a name. Later, `recall()` switches the current drawing state to one of those prepared profiles without consuming it.

### Recommended Direction

The preferred design is to make `canvas:state()` the single entry point for drawing-state management, and to keep `Canvas` itself focused on image operations and drawing commands.

`push()` and `pop()` should remain stack operations on the state controller. `capture()`, `recall()`, and `forget()` should be persistent preset operations on the same controller. This gives both concepts clear names and keeps their lifetimes distinct.

The default build should keep palette bank LUTs synchronized. Bank-local LUT behavior can remain available behind `TOFU_GRAPHICS_PALETTE_BANK_LUT_SYNC` for experiments and effects, but it should be documented as a semantic change rather than presented as a transparent optimization.


### State Object Creation

The state controller does not need to be created eagerly in the `Canvas` C constructor. A lazy approach is preferable because many canvases may never need direct state control. Creating a `State` object only when `canvas:state()` is first called avoids unnecessary userdata allocation and avoids cross-module construction from `canvas.c`.

The clean ownership model is to keep a weak Lua cache keyed by canvas:

```lua
local states <const> = setmetatable({}, { __mode = "k" })

function Canvas:state()
  local state = states[self]
  if not state then
    state = State.new(self)
    states[self] = state
  end
  return state
end
```

This cache makes repeated `canvas:state()` calls return the same controller while the canvas is alive, but it does not keep the canvas or state alive by itself. If both objects become unreachable, the weak entry can disappear.

The `State` userdata still needs to keep its canvas alive because it stores a borrowed `GL_Context_t *`. Otherwise user code could keep a `State` after the `Canvas` has been collected, leaving the state object with a dangling context pointer. The state object should therefore hold a strong reference to the canvas, following the same ownership style already used by objects such as banks, batches, and fonts:

```c
typedef struct State_Object_s {
    struct {
        const Canvas_Object_t *instance;
        luaX_Reference reference;
    } canvas;
    GL_Context_t *context;
} State_Object_t;
```

`State.new(canvas)` should store `canvas->context` and retain argument `1` with `luaX_ref()`. The state finalizer should release that reference with `luaX_unref()`. With this shape, the ownership graph is straightforward: the weak cache points from `Canvas` to `State` without owning either object, while `State` strongly owns the `Canvas` that owns the context.

Creating the state object eagerly from C is possible, but it is less attractive. `udt_newobject()` uses the current module metatable, so `canvas.c` cannot call it directly to create a `State` userdata. A cross-module helper would have to call `luaX_newobject()` with the explicit `tofu.graphics.state` metatable, push the state userdata on the stack, and then store it as a uservalue or registry reference on the canvas. Uservalues would be preferable to registry references because they are visible to the Lua garbage collector and would not create an uncollectable registry-reference cycle.

That eager route also requires the state metatable to exist before `Canvas.new()` creates a canvas. The lazy Lua cache avoids that ordering concern and keeps object creation in the state module, where the correct metatable is already available.

The helper name `from_canvas()` is not ideal because it sounds like a conversion. `for_canvas()` is semantically closer because the object is a live controller for a canvas, not a value derived from it. The best public API, however, is to avoid exposing either helper as the intended construction path. `canvas:state()` should be the documented way to get a state controller. The Lua wrapper can call `State.new(self)` internally, and the direct constructor can remain a low-level implementation detail rather than a named public convenience.
