TARGET=tofu

COMPILER=cc
CWARNINGS=-Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS=-O0 -g -DDEBUG -D_DEFAULT_SOURCE -DLUA_USE_LINUX -std=c99 -Iexternal
#CFLAGS=-O2 -g -DRELEASE -D_DEFAULT_SOURCE -DLUA_USE_LINUX -std=c99 -Iexternal

LINKER=cc
LFLAGS=-Wall -Wextra -Werror -Lexternal/GLFW -lglfw3 -lm  -ldl -lpthread -lrt -lX11

SOURCES:= $(wildcard src/*.c  src/core/*.c src/gl/*.c src/modules/*.c src/modules/graphics/*.c external/glad/*.c external/lua/*.c external/spleen/*.c external/stb/*.c)
INCLUDES:= $(wildcard src/*.h src/core/*.h src/gl/*.h src/modules/*.h src/modules/graphics/*.h external/glad/*.h external/GLFW/*.h external/lua/*.h external/spleen/*.h external/stb/*.h)
OBJECTS:= $(SOURCES:%.c=%.o)
RM=rm -f

default: $(TARGET)
all: default

$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): %.o : %.c $(INCLUDES) Makefile
	@$(COMPILER) $(CFLAGS) $(CWARNINGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

primitives: $(TARGET)
	@echo "Launching Primitives application!"
	./$(TARGET) ./demos/primitives

bunnymark: $(TARGET)
	@echo "Launching Bunnymark application!"
	./$(TARGET) ./demos/bunnymark

fire: $(TARGET)
	@echo "Launching Fire application!"
	./$(TARGET) ./demos/fire

tiled-map: $(TARGET)
	@echo "Launching Tiled-Map application!"
	./$(TARGET) ./demos/tiled-map

timers: $(TARGET)
	@echo "Launching Timers application!"
	./$(TARGET) ./demos/timers

postfx: $(TARGET)
	@echo "Launching PostFX application!"
	./$(TARGET) ./demos/postfx

spritestack: $(TARGET)
	@echo "Launching Sprite-Stack application!"
	./$(TARGET) ./demos/spritestack

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(RM) $(TARGET)
	@echo "Executable removed!"