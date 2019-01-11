TARGET=tofu

COMPILER=cc
CWARNINGS=-Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS=-O0 -DDEBUG -g -D_DEFAULT_SOURCE -std=c99 -Iexternal
# -O3

LINKER=cc
LFLAGS=-Wall -Wextra -Werror -Lexternal/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SOURCES:= $(wildcard src/*.c external/jsmn/*.c external/wren/*.c)
INCLUDES:= $(wildcard src/*.h external/raylib/*.h external/jsmn/*.h external/wren/*.h)
OBJECTS:= $(SOURCES:%.c=%.o)
rm=rm -f

default: $(TARGET)
all: default

$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): %.o : %.c
	@$(COMPILER) $(CFLAGS) $(CWARNINGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(TARGET)
	@echo "Executable removed!"