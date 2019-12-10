TARGET=tofu

# Use software renderer to use VALGRIND
# > export LIBGL_ALWAYS_SOFTWARE=1
# valgrind --track-origins=yes ./tofu ./demos/mode7/

ANALYZER=luacheck
AFLAGS=--no-self --std lua53 -q

# In case we want to embed pre-compiled script, we need to disable the `LUA_32BITS` compile flag!
#	@luac5.3 -o - $< | $(DUMPER) $(DFLAGS) > $@
DUMPER=hexdump
DFLAGS=-v -e '1/1 "0x%02X,"'

COMPILER=cc
CWARNINGS=-Wall -Wextra -Werror -Wno-unused-parameter -Wpedantic
CFLAGS=-Og -g -DDEBUG -D_DEFAULT_SOURCE -DLUA_32BITS -DLUA_FLOORN2I=1 -DLUA_USE_LINUX -DSTBI_ONLY_PNG -DSTBI_NO_STDIO -std=c99 -Isrc -Iexternal
#CFLAGS=-O3 -DRELEASE -D_DEFAULT_SOURCE -DLUA_32BITS -DLUA_FLOORN2I=1 -DLUA_USE_LINUX -DSTBI_ONLY_PNG -DSTBI_NO_STDIO -std=c99 -Isrc -Iexternal
# -Ofast => -O3 -ffast-math
# -Os => -O2, favouring size

LINKER=cc
LFLAGS=-Wall -Wextra -Werror -Lexternal/GLFW -lglfw3 -lm  -ldl -lpthread -lrt -lX11

SOURCES:= $(wildcard src/*.c src/core/*.c src/core/io/*.c src/core/io/display/*.c src/core/vm/*.c src/core/vm/modules/*.c src/core/vm/modules/resources/*.c src/libs/*.c src/libs/gl/*.c external/glad/*.c external/GLFW/*.c external/lua/*.c external/miniaudio/*.c external/spleen/*.c external/stb/*.c)
INCLUDES:= $(wildcard src/*.h src/core/*.h src/core/io/*.h src/core/io/display/*.h src/core/vm/*.h src/core/vm/modules/*.h src/core/vm/modules/resources/*.h src/libs/*.h src/libs/gl/*.h external/glad/*.h external/GLFW/*.h external/lua/*.h external/miniaudio/*.h external/spleen/*.h external/stb/*.h)
OBJECTS:= $(SOURCES:%.c=%.o)
SCRIPTS:= $(wildcard src/core/vm/*.lua src/core/vm/modules/*.lua)
BLOBS:= $(SCRIPTS:%.lua=%.inc)
RM=rm -f

default: $(TARGET)
all: default

$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

# The dependency upon `Makefile` is redundant, since scripts are bound to it.
$(OBJECTS): %.o : %.c $(BLOBS) $(INCLUDES) Makefile
	@$(COMPILER) $(CFLAGS) $(CWARNINGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# Define a rule to automatically convert `.lua` script into an embeddable-ready `.inc` file.
# `.inc` files also depend upon `Makefile` to be rebuild in case of tweakings.
$(BLOBS): %.inc: %.lua Makefile
	@$(ANALYZER) $(AFLAGS) $<
	@$(DUMPER) $(DFLAGS) $< > $@
	@echo "Generated "$@" from "$<" successfully!"

primitives: $(TARGET)
	@echo "Launching *primitives* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/primitives
	./$(TARGET) ./demos/primitives

bunnymark: $(TARGET)
	@echo "Launching *bunnymark* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/bunnymark
	@./$(TARGET) ./demos/bunnymark

fire: $(TARGET)
	@echo "Launching *fire* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/fire
	@./$(TARGET) ./demos/fire

tiled-map: $(TARGET)
	@echo "Launching *tiled-map* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/tiled-map
	@./$(TARGET) ./demos/tiled-map

timers: $(TARGET)
	@echo "Launching *timers* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/timers
	@./$(TARGET) ./demos/timers

postfx: $(TARGET)
	@echo "Launching *postfx* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/postfx
	@./$(TARGET) ./demos/postfx

spritestack: $(TARGET)
	@echo "Launching *spritestack* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/spritestack
	@./$(TARGET) ./demos/spritestack

palette: $(TARGET)
	@echo "Launching *palette* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/palette
	@./$(TARGET) ./demos/palette

mode7: $(TARGET)
	@echo "Launching *mode7* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/mode7
	@./$(TARGET) ./demos/mode7

snake: $(TARGET)
	@echo "Launching *snake* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/snake
	@./$(TARGET) ./demos/snake

shades: $(TARGET)
	@echo "Launching *shades* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/shades
	@./$(TARGET) ./demos/shades

gamepad: $(TARGET)
	@echo "Launching *gamepad* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/gamepad
	@./$(TARGET) ./demos/gamepad

valgrind: $(TARGET)
	@echo "Valgrind *$(DEMO)* application!"
	@valgrind --track-origins=yes --leak-check=full env LIBGL_ALWAYS_SOFTWARE=1 ./$(TARGET) ./demos/$(DEMO)

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(BLOBS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(RM) $(TARGET)
	@echo "Executable removed!"