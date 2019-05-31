TARGET=tofu

COMPILER=cc
CWARNINGS=-Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS=-O0 -DDEBUG -g -D_DEFAULT_SOURCE -std=c99 -Iexternal
#CFLAGS=-O2 -g -D_DEFAULT_SOURCE -std=c99 -Iexternal

LINKER=cc
LFLAGS=-Wall -Wextra -Werror -Lexternal/GLFW -lglfw3 -lm  -ldl -lpthread -lrt -lX11

SOURCES:= $(wildcard src/*.c  src/core/*.c src/gl/*.c src/modules/*.c src/modules/graphics/*.c external/glad/*.c external/jsmn/*.c external/spleen/*.c external/stb/*.c external/wren/*.c)
INCLUDES:= $(wildcard src/*.h src/core/*.h src/gl/*.h src/modules/*.h src/modules/graphics/*.h external/glad/*.h external/GLFW/*.h external/jsmn/*.h external/spleen/*.h external/stb/*.h external/wren/*.h)
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

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(RM) $(TARGET)
	@echo "Executable removed!"