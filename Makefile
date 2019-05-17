TARGET=tofu

COMPILER=cc
CWARNINGS=-Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS=-O0 -DDEBUG -g -D_DEFAULT_SOURCE -std=c99 -Iexternal
#CFLAGS=-O2 -g -D_DEFAULT_SOURCE -std=c99 -Iexternal

LINKER=cc
LFLAGS=-Wall -Wextra -Werror -Lexternal/GLFW -lglfw3 -lm  -ldl -lpthread -lrt -lX11

SOURCES:= $(wildcard src/*.c  src/core/*.c src/modules/*.c src/modules/graphics/*.c external/jsmn/*.c external/glad/*.c external/wren/*.c)
INCLUDES:= $(wildcard src/*.h src/core/*.h src/modules/*.h src/modules/graphics/*.h external/jsmn/*.h external/wren/*.h external/GLFW/*.h external/glad/*.h)
OBJECTS:= $(SOURCES:%.c=%.o)
rm=rm -f

default: $(TARGET)
all: default

$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): %.o : %.c $(INCLUDES) Makefile
	@$(COMPILER) $(CFLAGS) $(CWARNINGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

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
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(TARGET)
	@echo "Executable removed!"