TARGET=tofu

COMPILER=gcc
CFLAGS=-g -Wall -Wextra -Iexternal/include

LINKER=gcc
LFLAGS=-Wall -Wextra -Lexternal/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SOURCES:= $(wildcard src/*.c external/include/jsmn/*.c)
INCLUDES:= $(wildcard src/*.h external/include/raylib/*.h external/include/jsmn/*.h)
OBJECTS:= $(SOURCES:%.c=%.o)
rm=rm -f

default: $(TARGET)
all: default

$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): %.o : %.c
	@$(COMPILER) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(TARGET)
	@echo "Executable removed!"