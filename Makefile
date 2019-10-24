TARGET=tofu

# Use software renderer to use VALGRIND
# > export LIBGL_ALWAYS_SOFTWARE=1
# valgrind --track-origins=yes ./tofu ./demos/mode7/

ANALYZER=luacheck
AFLAGS=--no-self --std lua53 -q

COMPILER=cc
CWARNINGS=-Wall -Wextra -Werror -Wno-unused-parameter -Wpedantic 
CFLAGS=-Og -g -DDEBUG -D_DEFAULT_SOURCE -DLUA_32BITS -DLUA_FLOORN2I=1 -DLUA_USE_LINUX -DSTBI_ONLY_PNG -DSTBI_NO_STDIO -std=c99 -Iexternal
#CFLAGS=-O3 -DRELEASE -D_DEFAULT_SOURCE -DLUA_32BITS -DLUA_FLOORN2I=1 -DLUA_USE_LINUX -DSTBI_ONLY_PNG -DSTBI_NO_STDIO -std=c99 -Iexternal
# -Ofast => -O3 -ffast-math
# -Os => -O2, favouring size

LINKER=cc
LFLAGS=-Wall -Wextra -Werror -Lexternal/GLFW -lglfw3 -lm  -ldl -lpthread -lrt -lX11

SOURCES:= $(wildcard src/*.c  src/core/*.c src/gl/*.c src/modules/*.c src/modules/graphics/*.c external/glad/*.c external/lua/*.c external/spleen/*.c external/stb/*.c)
INCLUDES:= $(wildcard src/*.h src/core/*.h src/gl/*.h src/modules/*.h src/modules/graphics/*.h external/glad/*.h external/GLFW/*.h external/lua/*.h external/spleen/*.h external/stb/*.h)
OBJECTS:= $(SOURCES:%.c=%.o)
SCRIPTS:= $(wildcard src/modules/*.lua)
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
	@xxd -i $< | sed -e 's/src_modules//g' -e 's/unsigned/static unsigned/g' > $@
	@echo "Generated "$@" from "$<" successfully!"

primitives: $(TARGET)
	@echo "Launching Primitives application!"
	@$(ANALYZER) $(AFLAGS) ./demos/primitives
	./$(TARGET) ./demos/primitives

bunnymark: $(TARGET)
	@echo "Launching Bunnymark application!"
	@$(ANALYZER) $(AFLAGS) ./demos/bunnymark
	@./$(TARGET) ./demos/bunnymark

fire: $(TARGET)
	@echo "Launching Fire application!"
	@$(ANALYZER) $(AFLAGS) ./demos/fire
	@./$(TARGET) ./demos/fire

tiled-map: $(TARGET)
	@echo "Launching Tiled-Map application!"
	@$(ANALYZER) $(AFLAGS) ./demos/tiled-map
	@./$(TARGET) ./demos/tiled-map

timers: $(TARGET)
	@echo "Launching Timers application!"
	@$(ANALYZER) $(AFLAGS) ./demos/timers
	@./$(TARGET) ./demos/timers

postfx: $(TARGET)
	@echo "Launching PostFX application!"
	@$(ANALYZER) $(AFLAGS) ./demos/postfx
	@./$(TARGET) ./demos/postfx

spritestack: $(TARGET)
	@echo "Launching Sprite-Stack application!"
	@$(ANALYZER) $(AFLAGS) ./demos/spritestack
	@./$(TARGET) ./demos/spritestack

palette: $(TARGET)
	@echo "Launching Palette application!"
	@$(ANALYZER) $(AFLAGS) ./demos/palette
	@./$(TARGET) ./demos/palette

mode7: $(TARGET)
	@echo "Launching Mode7 application!"
	@$(ANALYZER) $(AFLAGS) ./demos/mode7
	@./$(TARGET) ./demos/mode7

valgrind: $(TARGET)
	@echo "Valgrind Palette application!"
	@valgrind --leak-check=full env LIBGL_ALWAYS_SOFTWARE=1 ./$(TARGET) ./demos/palette

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(BLOBS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(RM) $(TARGET)
	@echo "Executable removed!"