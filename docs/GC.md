automatic mode, both generation and incremental makes the memory increase
continuous mode, memory is kept constant -- incremental seems a bit less cpu intensive
manual mode, the periodic collect take up to 0.003ms

no apparent costs in calling "step" at each update. but a second test indicated a drop of ~3 FPS. Perhaps calling once every 5 (or 0.1) seconds, and let me memory grow a bit?

memory increase in bunnymark 50KiB every 15 seconds. Garbage collection is to be preferred.

the continuous mode gives the same result of the periodic full-GC.

Disabling GC debug and periodic collect in release build.
