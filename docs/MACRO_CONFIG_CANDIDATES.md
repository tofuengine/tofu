# Translation-unit macro config candidates

### Scope

This note collects compile-time macros found under `src` that are local to one translation unit, or are effectively hidden local switches, and that can change engine behavior, rendering output, runtime policy, diagnostics, or performance. The purpose is to decide which of these should be promoted to `src/core/config.h` under stable `TOFU_*` names.

The scan intentionally excludes include guards, `_LOG_TAG` definitions, small expression helpers, third-party implementation macros, pure ABI constants, and macros that are already represented in `src/core/config.h`. When a macro is currently named with a leading underscore, promotion should also rename it to a project-level name, for example from `_BOOT_SCRIPT` to something like `TOFU_INTERPRETER_BOOT_SCRIPT`.

### Primary candidates

| Macro | Location | Current behavior | Promotion note |
|---|---|---|---|
| `_LUAX_RTTI` | `src/libs/luax.c:44` | Enables Lua userdata type tagging in `DEBUG` builds unless `LUAX_NO_RTTI` is defined. This changes userdata layout and makes object type checks stricter. | Good candidate for an interpreter or LuaX section because it is a real safety and memory-layout policy. |
| `_WAIT_SLOT` | `src/core/soy/time.c:74` | Keeps the final wait slot at `1ms` and uses a semi-busy wait/yield for the remainder. This affects frame pacing precision and CPU usage. | Good candidate near the existing core timing options. |
| `_USE_COLORS` | `src/libs/log.c:45` | Enables ANSI-colored log output on Linux. | Low priority, but it is a platform logging policy rather than an implementation detail. |
| `_STREAMING_BUFFER_SIZE_IN_FRAMES` | `src/libs/sl/music.c:41`, `src/libs/sl/module.c:41` | Hard-codes streaming audio buffers to one second of decoded data. This affects memory use, I/O cadence, and underrun tolerance. | Good candidate in the sound section. Music and modules currently share the same local value. |
| `_STREAMING_BUFFER_CHUNK_IN_FRAMES` | `src/libs/sl/music.c:44`, `src/libs/sl/module.c:44` | Limits each producer update to one quarter of the streaming buffer. This affects how aggressively streaming sources refill buffers. | Good candidate if streaming buffer size is promoted. |
| `_MODULE_OUTPUT_FORMAT` and `_MODULE_OUTPUT_*` | `src/libs/sl/module.c:47` | Forces module decoding to signed 16-bit stereo before conversion/mixing. | Lower priority unless module decode format or channel count should be configurable. |
| `_SAMPLE_MAX_LENGTH_IN_SECONDS` | `src/libs/sl/sample.c:40` | Rejects decoded FLAC samples longer than `10.0s`. | Strong candidate. A config value, with `#undef` meaning unlimited, would make the policy explicit. |
| `_MIN_SPEED_VALUE` | `src/libs/sl/props.c:39` | Clamps playback speed to the min/max standard sample-rate ratio supported by miniaudio. | Behavior-visible, but derived from backend limits. It may be better documented than configured. |
| `_KEYBOARD_A_CONTROLLER_ID` | `src/systems/input.c:399` | Maps the first keyboard-emulated controller to controller slot `0`. | Good candidate because keyboard controller emulation is already configured in `config.h`. |
| `_KEYBOARD_B_CONTROLLER_ID` | `src/systems/input.c:400` | Maps the second keyboard-emulated controller to controller slot `1`. | Same as above. |
| `_CURSOR_CONTROLLER_ID` | `src/systems/input.c:432` | Maps controller-to-cursor button emulation to controller slot `0`. | Good candidate because cursor emulation is already configured in `config.h`. |
| `_BOOT_SCRIPT` | `src/systems/interpreter.c:51` | Hard-codes the Lua boot module name to `"boot"`. | Strong candidate, likely `TOFU_INTERPRETER_BOOT_SCRIPT`. |
| `_EMA_ALPHA` | `src/systems/environment.c:50` | Hard-codes exponential smoothing for FPS, frame-time, and heap statistics. | Good candidate near the existing statistics options. |
| `_ENGINE_EPOCH` | `src/core/engine.c:56` | Sets the engine clock to `4294967296.0` seconds to preserve stable floating-point precision. | Behavior-relevant, but probably best left documented unless there is a real need to change it. |

### Hidden local switches

These are easy to miss because some are not actively defined in the file. They are still translation-unit local behavior switches because the code tests them locally and they can be enabled only by external compiler flags or by uncommenting local defines.

| Macro | Location | Current behavior | Promotion note |
|---|---|---|---|
| `_NO_MEMSET_MEMCPY` | `src/libs/gl/surface.c:206` | Switches `GL_surface_clear()` from `memset()` to a manual pixel loop. | Good candidate only if this path is still useful for benchmarking, portability, or platform-specific tuning. |
| `_BRESENHAM_LINES` | `src/libs/gl/primitive.c:146` | Switches line rasterization from the default DDA path to Bresenham. This can change exact line pixels and performance. | Good candidate if line rasterization semantics should be selectable. |
| `_BRANCHLESS_BLIT_EXPERIMENTAL` | `src/libs/gl/blit.c:48` | Enables branchless transparent-pixel blending in the blit paths. | Potential candidate, but it is explicitly experimental and should be validated before promotion. |
| `_BRANCHLESS_BLIT_EXPERIMENTAL_XOR` | `src/libs/gl/blit.c:49` | Selects the XOR blend expression inside the branchless blit experiment. | Do not promote as-is. The guard currently uses `#ifdef defined(...)`, which should be fixed before any config decision. |
| `_DETACH_XFORM_TABLE` | `src/libs/gl/xform.c:233` | Detaches the xform table pointer after end-of-data to skip future table checks. | A small performance switch. Consider promoting only if measured useful. |
| `_CLIP_OFFSET` | `src/libs/gl/xform.c:243` | Applies `fmodf()` to xform offsets to reduce cancellation when offsets are large. This changes numeric behavior and can affect rendering. | Good candidate if large-offset transform behavior matters across builds. |

### Fixed-point module knobs

The fixed-point library now exposes three header-level switches. They are not translation-unit local in the strict sense, but they are hidden module configuration that directly affects the scaler and roto-scaler because `src/libs/gl/blit.c` uses the `FIXED32_*` conversion macros whenever `TOFU_GRAPHICS_USE_FIXED_MATH` is enabled. These values must be consistent across every translation unit that exchanges raw `fixed32_t` values.

| Macro | Location | Current behavior | Promotion note |
|---|---|---|---|
| `FIXED32_FRACTIONAL_BITS` | `src/libs/fpmath.h:52` | Defaults to `16` and accepts values from `1` through `30`. It controls the precision/range tradeoff of every fixed-point value. In the blitters it changes the quantization of source origins and per-pixel increments, so it can change sampled pixels and accumulated drift. | Strong candidate for a graphics setting such as `TOFU_GRAPHICS_FIXED_FRACTIONAL_BITS`, conditional on `TOFU_GRAPHICS_USE_FIXED_MATH`. Changing it is a representation-level decision, not merely a local optimization. |
| `FIXED32_USE_64_BIT` | `src/libs/fpmath.h:66` | Uses 64-bit intermediates in integer-to-fixed conversion and integer rounding. This avoids intermediate overflow across more of the raw `fixed32_t` domain, at a possible cost on targets where 64-bit integer operations are slower. It does not make an out-of-range final fixed-point result representable. | Reasonable safety/performance candidate, for example `TOFU_GRAPHICS_FIXED_WIDE_INTERMEDIATES`. Keep it build-wide if `fixed32_t` operations spread beyond the blitter. |
| `FIXED32_NO_ROUNDING` | `src/libs/fpmath.h:174` | Changes the convenience conversion macros from nearest rounding to truncation. In the current blitters, `FIXED32_FROM_FLOAT()` is used for source origins, bounds, and incremental deltas, so enabling this macro can alter sampling phase and exact raster output. | Candidate only with explicit pixel-output tests. Prefer a positive project name such as `TOFU_GRAPHICS_FIXED_ROUND_CONVERSIONS` rather than promoting the negative library name directly. |

### Already represented in config

Some macros looked local during review but are already in `src/core/config.h`, or are derived directly from macros already there. They do not need promotion, though their names or comments can still be cleaned up.

`TOFU_GRAPHICS_USE_FIXED_MATH` is already defined in `src/core/config.h:369` and used in `src/libs/gl/blit.c` to switch the scaler and roto-scaler to fixed-point increments.

`TOFU_GRAPHICS_NO_IFLOORF`, `TOFU_GRAPHICS_OPTIMIZE_AABB_ROTATIONS`, `TOFU_GRAPHICS_BRANCHLESS_CALCULATIONS`, `TOFU_GRAPHICS_BLIT_FAST_PATH`, `TOFU_GRAPHICS_XFORM_TRANSPARENCY`, `TOFU_GRAPHICS_FIX_RASTERIZER_WINDING`, and the other major graphics switches are already centralized in `config.h`.

`TOFU_INTERPRETER_READER_BUFFER_SIZE`, `TOFU_INTERPRETER_PROTECTED_CALLS`, `TOFU_INTERPRETER_CUSTOM_TRACEBACK`, and `TOFU_INTERPRETER_PARTIAL_OBJECT` already control the interpreter paths where the local stack-index and reader-buffer macros are used.

`TOFU_INPUT_CONTROLLER_IS_EMULATED` and `TOFU_INPUT_CURSOR_IS_EMULATED` already control whether emulation is compiled in. The remaining local input candidates are the concrete controller slot IDs used when those features are enabled.

`FMATH_FAST_OPERATIONS` and `IMATH_FAST_OPERATIONS` are module-level aliases of `TOFU_CORE_FAST_MATH`, and release builds also define their `FMATH_INLINE_FAST_OPERATIONS` and `IMATH_INLINE_FAST_OPERATIONS` variants in the Makefile. The inline variants change linkage and call overhead rather than intended numerical behavior, so they belong to build policy rather than `config.h`.

### Scaler and roto-scaler fast-path correctness

`TOFU_GRAPHICS_BLIT_FAST_PATH` currently exposes a behavioral difference between the scaler and the roto-scaler. When `GL_context_sprite_sr()` runs with zero rotation, its inverse transform is still derived from a half-pixel-centered destination pivot. In contrast, `GL_context_sprite_s()` accepts an integer top-left destination position and assumes that this position lies exactly on a destination pixel edge.

The zero-rotation fast path in `GL_sheet_blit_sr()` converts the continuous anchor-relative destination origin into a `GL_Point_t` before calling `GL_context_sprite_s()`. It calculates the fractional top-left position and then applies `IFLOORF()`. This preserves the approximate destination bounds but discards the fractional destination phase that determines which source texel each destination pixel center should sample. Consequently, a sprite whose logical position is unchanged can appear to wiggle as its scale changes, while the general roto-scaler produces more stable placement at rotation zero.

Some changes inside a nearest-neighbor-scaled image are unavoidable when the scale changes continuously: destination pixels must occasionally start sampling different source texels. The bug under discussion is the avoidable whole-sprite positional or sampling-phase jump introduced by converting the anchored fractional origin to an integer top-left coordinate.

The intended long-term fix is to give the scaler native anchor support and to separate the anchor from the rotation pivot. The anchor should describe which point of the scaled image is placed at the destination position. The pivot should independently describe the point around which rotation is performed. This also avoids forcing the sheet layer to translate an anchor-relative position into a rounded top-left coordinate.

Adding an anchor argument alone is not sufficient if the implementation still rounds the anchor-adjusted origin before entering the scaler. The fractional destination phase must survive into the scaler's initial source-coordinate calculation. At zero rotation, the scaler should use the same inverse-mapping model as the roto-scaler:

```c
u = source_anchor_x + (destination_pixel_center_x - destination_position_x) / scale_x;
v = source_anchor_y + (destination_pixel_center_y - destination_position_y) / scale_y;
```

With that model, the zero-rotation fast path can call the anchored scaler without changing the sampling phase. The anchor and pivot transform order must be documented explicitly, especially for negative scale, non-central anchors, odd and even sprite dimensions, and rotations equivalent to zero.

Until native anchored scaling is implemented, the zero-rotation fast path should not be assumed to be pixel-equivalent to the general roto-scaler for fractional scales. Disabling that fast path for affected anchored scaling calls is the conservative correctness option. Promotion or continued default enablement of `TOFU_GRAPHICS_BLIT_FAST_PATH` should therefore account for this known visual difference.

### Adjacent shared macros

These are not translation-unit local because they live in headers and affect multiple users, but they are behavior-defining constants that may deserve a separate configuration review.

| Macro | Location | Behavior |
|---|---|---|
| `LUAX_RUNTIME_CHECKS` | `src/libs/luax.h:45` | Enables Lua API argument and stack checks in `DEBUG` builds. |
| `LUAX_ARITY_OVERLOAD_ONLY` | `src/libs/luax.h:133` | Selects arity-only overload dispatch instead of type-signature dispatch. |
| `LUAX_STRICT_INTEGERS` | `src/libs/luax.h:166` | Requires Lua integer values for integer arguments instead of accepting generic numbers. |
| `SINCOS_PERIOD` | `src/libs/sincos.h:41` | Sets the trigonometric lookup-table period exposed to Lua as `Math.SINCOS_PERIOD`. |
| `SL_FRAMES_PER_SECOND` | `src/libs/sl/common.h:56` | Sets the sound subsystem sample rate. |
| `SL_MIXING_BUFFER_SIZE_IN_FRAMES` | `src/libs/sl/common.h:60` | Sets the internal sound mixing buffer size. |
| `SL_GROUPS_AMOUNT` | `src/libs/sl/common.h:64` | Sets the number of sound groups. |
| `INPUT_CONTROLLERS_COUNT` | `src/systems/input.h:163` | Sets the number of controller slots. |
| `XOR_MAX_KEY_LENGTH` | `src/libs/xor.h:44` | Limits the XOR stream cipher key length used by PAK decryption. |
| `IMG_PALETTE_MAX_COLORS` | `src/libs/image.h:54` | Sets the custom image format palette size. |
| `GL_PALETTE_AVAILABLE_COLORS`, `GL_PALETTE_MAX_COLORS`, `GL_PALETTE_MAX_BANKS`, `GL_PALETTE_BANK_SHIFT` | `src/libs/gl/common.h:57` | Define the graphics palette and bank layout. These are deeply tied to pixel format and image semantics. |

### Shader-local knobs

The GLSL files under `src/kernal/assets/glsl` contain many preprocessor constants that visibly change post-processing output. They should not be moved directly to C `config.h` unless the shader compilation path gains a preprocessing step that injects C configuration into shader sources. Uniforms or shader parameters would probably be a better fit for runtime-selectable values.

The relevant shader files are `zfast-crt.glsl`, `zfast-crt-vertical.glsl`, `crt-pi.glsl`, `crt-pi-vertical.glsl`, `zfast-lcd.glsl`, `scanline-fract.glsl`, and `scanlines-sine-abs.glsl`. Their knobs include scanline strength, gamma, curvature, mask type, brightness boost, blur amount, LCD border strength, and similar visual parameters. Several of these shaders already include commented `#pragma parameter` lines, which suggests they came from shader systems where these values were expected to be user-tunable.

### Low-priority or excluded local macros

Several local macros were reviewed but do not look like `config.h` candidates. `_MAX_DATE_LENGTH` in `src/modules/system.c` is a fixed buffer size for date formatting. `_INC_IF_VALID` in `src/modules/program.c` is a one-off helper. `_REGION_*` in `src/libs/gl/primitive.c` are Cohen-Sutherland clipping bit masks. `_LUT_LENGTH` in `src/libs/sincos.c` is derived from `SINCOS_PERIOD`. `_PIXEL_FORMAT`, `_PIXEL_TYPE`, and `_PIXEL_BPP` in `src/systems/display.c` are derived from `TOFU_GRAPHICS_PIXEL_FORMAT`.

`_LUAX_CHECK_STACK_DUMP` in `src/libs/luax.c` only adds an internal positive/negative-index consistency check to explicit Lua stack dumps. If retained, it should follow an existing debug or defensive-check setting instead of becoming an independent project option.

`_VERBOSE_DEBUG` in `src/modules/vector2d.c` is a local commented debug toggle. It should not be promoted as a new option; if those logs are useful, they should probably be folded under `TOFU_CORE_VERBOSE_DEBUG`.

