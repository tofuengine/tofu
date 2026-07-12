# Canvas State Creation

## Option 1

The simpler and safer design is: let State.new(canvas) create the C State userdata, and make State_Object_t hold a registry reference to the canvas. Then State keeps the canvas/context alive.

  Right now src/modules/state.c:86 stores only:

  State_Object_t {
      GL_Context_t *context;
  }

  That means a State can outlive its Canvas and keep a dangling context pointer. I would change it to something like:

  typedef struct State_Object_s {
      struct {
          const Canvas_Object_t *instance;
          luaX_Reference reference;
      } canvas;
      GL_Context_t *context;
  } State_Object_t;

  Then in state_new_1o_1o():

  const Canvas_Object_t *canvas = LUAX_OBJECT(L, 1, OBJECT_TYPE_CANVAS);

  State_Object_t *self = udt_newobject(L, sizeof(State_Object_t), &(State_Object_t){
          .canvas = {
              .instance = canvas,
              .reference = luaX_ref(L, 1)
          },
          .context = canvas->context
      }, OBJECT_TYPE_STATE);

  And in state_gc_1o_0():

  luaX_unref(L, self->canvas.reference);

  Then canvas:state() can stay in Lua and cache lazily:

  local states <const> = setmetatable({}, { __mode = "k" }) -- Weak table, don't keep objects alive

  function Canvas:state()
    local state = states[self]
    if not state then
      state = State.from_canvas(self)
      states[self] = state
    end
    return state
  end

  This avoids cross-module construction from canvas.c, avoids storing a strong Canvas -> State -> Canvas cycle, and keeps State safe if user code stores it
  separately.

  If you create the State inside canvas_new_1o_1o(), udt_newobject() is not enough from canvas.c, because it will use the canvas module metatable. You would
  need an explicit cross-module constructor helper that sets the "tofu.graphics.state" metatable, then store a registry reference in Canvas_Object_t. That is
  doable, but it introduces lifetime edge cases and likely needs invalidation in canvas_gc. I would use the lazy Lua cache plus a canvas reference in
  State_Object_t instead.

## Option 2

The helper I’d add is a state.c “friend constructor” that creates a State userdata with the state module metatable, pushes it on the Lua stack, and links it
  to the canvas through userdata uservalues, not registry refs.

  Do not use udt_newobject() from canvas.c for this. It would use the canvas module metatable. The helper should call luaX_newobject() with the explicit state
  metatable name.

  Something like this in state.h:

  extern void state_new_from_canvas(lua_State *L, int canvas_index);

  And in state.c:

  #define _STATE_MODULE_NAME "tofu.graphics.state"

  void state_new_from_canvas(lua_State *L, int canvas_index)
  {
      canvas_index = lua_absindex(L, canvas_index);

      const Canvas_Object_t *canvas =
          (const Canvas_Object_t *)LUAX_OBJECT(L, canvas_index, OBJECT_TYPE_CANVAS);

      State_Object_t *self = (State_Object_t *)luaX_newobject(L,
          sizeof(State_Object_t),
          &(State_Object_t){
              .context = canvas->context
          },
          OBJECT_TYPE_STATE,
          _STATE_MODULE_NAME);

      lua_pushvalue(L, canvas_index);
      lua_setiuservalue(L, -2, 1); /* State keeps Canvas alive, GC-visible. */

      LOG_D("state %p allocated w/ context %p for canvas %p", self, self->context, canvas);
  }

  Then state_new_1o_1o() can just call this helper and return 1.

  In canvas_new_1o_1o(), after creating the canvas userdata:

  state_new_from_canvas(L, -1);  /* Canvas at -2, State at -1. */
  lua_setiuservalue(L, -2, 1);   /* Canvas keeps cached State, GC-visible. */

  return 1;                      /* Canvas remains on top. */

  Then a C canvas_state_1o_1o() method becomes trivial:

  lua_getiuservalue(L, 1, 1);
  return 1;

  The reason I’d use uservalues instead of luaX_ref() here is that a registry ref cycle would leak. Uservalues are visible to Lua’s GC, so a Canvas <-> State
  cycle can still be collected. One caveat: the "tofu.graphics.state" metatable must exist before this helper runs, so keep the Lua wrapper requiring
  tofu.graphics.state, or otherwise force-load the state module before constructing canvases.
