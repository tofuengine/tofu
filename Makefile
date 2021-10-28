ifeq ($(PLATFORM),windows)
	ifeq ($(ARCHITECTURE),x64)
		TARGET=tofu_x64.exe
	else
		TARGET=tofu_x32.exe
	endif
else ifeq ($(PLATFORM),rpi)
	TARGET=tofu-rpi_x32
else
	TARGET=tofu
endif

# CppCheck
# 	cppcheck --enable=all ./src > /dev/null

# Use software renderer to use VALGRIND
#   export LIBGL_ALWAYS_SOFTWARE=1
#   valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./tofu --path=./demos/splash

ANALYZER=luacheck
AFLAGS=--no-self --std lua53 -q

RM=rm -f

# In case we want to embed pre-compiled script, we need to disable the `LUA_32BITS` compile flag!
#	@luac5.3 -o - $< | $(DUMPER) $(DFLAGS) > $@
DUMPER=hexdump
DFLAGS=-v -e '1/1 "0x%02X,"'

ifeq ($(PLATFORM),windows)
	ifeq ($(ARCHITECTURE),x64)
		COMPILER=x86_64-w64-mingw32-gcc
	else
		COMPILER=i686-w64-mingw32-gcc
	endif
else
	COMPILER=gcc
endif
CWARNINGS=-std=c99 -Wall -Wextra -Werror -Winline -Wlogical-op -Wno-unused-parameter -Wpedantic -Wpointer-arith -Wstrict-prototypes -Wshadow -Wunreachable-code -Wwrite-strings
#CWARNINGS+=-Wfloat-equal
CFLAGS=-D_DEFAULT_SOURCE -DLUA_32BITS -DLUA_FLOORN2I=F2Ifloor -DGIF_FLIP_VERT -DSTB_IMAGE_WRITE_FLIP_VERTICALLY -DSTBI_ONLY_PNG -DSTBI_NO_STDIO -DDR_FLAC_NO_STDIO -DMA_NO_DECODING -DMA_NO_ENCODING -DMA_NO_GENERATION -DLIBXMP_BUILDING_STATIC -Isrc -Iexternal
ifneq ($(PLATFORM),windows)
	CFLAGS+=-DLUA_USE_LINUX
endif
ifeq ($(BUILD),release)
# -Ofast => -O3 -ffast-math
# -Os => -O2, favouring size
	COPTS=-O3 -ffast-math -DNDEBUG -fomit-frame-pointer
else ifeq ($(BUILD),profile)
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG -DPROFILE -pg
else ifeq ($(BUILD),sanitize)
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG -fsanitize=address -fno-omit-frame-pointer
else
#	COPTS=-Og -ggdb3 -DDEBUG
#	COPTS=-O0 -ggdb3 -DDEBUG
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG
endif

ifeq ($(PLATFORM),windows)
	ifeq ($(ARCHITECTURE),x64)
		LINKER=x86_64-w64-mingw32-gcc
		LFLAGS=-Lexternal/GLFW/windows/x64 -lglfw3 -lgdi32 -lpsapi
	else
		LINKER=i686-w64-mingw32-gcc
		LFLAGS=-Lexternal/GLFW/windows/x32 -lglfw3 -lgdi32 -lpsapi
	endif
else ifeq ($(PLATFORM),rpi)
	LINKER=gcc
	LFLAGS=-Lexternal/GLFW/rpi/x32 -lglfw3 -lm -lpthread -lX11 -ldl
else
	LINKER=gcc
	LFLAGS=-Lexternal/GLFW/linux/x64 -lglfw3 -lm -lpthread -lX11 -ldl
endif
LWARNINGS=-Wall -Wextra -Werror
ifeq ($(BUILD),release)
	LOPTS=-fomit-frame-pointer
else ifeq ($(BUILD),profile)
	LOPTS=-pg
else ifeq ($(BUILD),sanitize)
	LOPTS=-fsanitize=address -fno-omit-frame-pointer
else
	LOPTS=
endif

# Source files list (src)
SOURCES:=$(wildcard src/*.c)
SOURCES+=$(wildcard src/core/*.c)
SOURCES+=$(wildcard src/core/systems/*.c)
SOURCES+=$(wildcard src/core/systems/display/*.c)
SOURCES+=$(wildcard src/core/systems/vm/*.c)
SOURCES+=$(wildcard src/core/systems/vm/modules/*.c)
SOURCES+=$(wildcard src/core/systems/vm/modules/utils/*.c)
SOURCES+=$(wildcard src/core/utils/*.c)
SOURCES+=$(wildcard src/libs/*.c)
SOURCES+=$(wildcard src/libs/fs/*.c)
SOURCES+=$(wildcard src/libs/gl/*.c)
SOURCES+=$(wildcard src/libs/sl/*.c)
SOURCES+=$(wildcard src/resources/*.c)
# Source files list (external)
SOURCES+=$(wildcard external/dr_libs/*.c)
SOURCES+=$(wildcard external/gif-h/*.c)
SOURCES+=$(wildcard external/glad/*.c)
SOURCES+=$(wildcard external/lua/*.c)
SOURCES+=$(wildcard external/miniaudio/*.c)
SOURCES+=$(wildcard external/spleen/*.c)
SOURCES+=$(wildcard external/xmp-lite/*.c)
SOURCES+=$(wildcard external/noise/*.c)
SOURCES+=$(wildcard external/chipmunk/*.c)
# Include files list (src)
INCLUDES:=$(wildcard src/*.h)
INCLUDES:=$(wildcard src/core/*.h)
INCLUDES:=$(wildcard src/core/systems/*.h)
INCLUDES:=$(wildcard src/core/systems/display/*.h)
INCLUDES:=$(wildcard src/core/systems/vm/*.h)
INCLUDES:=$(wildcard src/core/systems/vm/modules/*.h)
INCLUDES:=$(wildcard src/core/systems/vm/modules/utils/*.h)
INCLUDES+=$(wildcard src/core/utils/*.h)
INCLUDES:=$(wildcard src/libs/*.h)
INCLUDES:=$(wildcard src/libs/fs/*.h)
INCLUDES:=$(wildcard src/libs/gl/*.h)
INCLUDES:=$(wildcard src/libs/sl/*.h)
INCLUDES:=$(wildcard src/resources/*.h)
# Include files list (external)
INCLUDES+=$(wildcard external/dr_libs/*.h)
INCLUDES+=$(wildcard external/gif-h/*.h)
INCLUDES+=$(wildcard external/glad/*.h)
INCLUDES+=$(wildcard external/GLFW/*.h)
INCLUDES+=$(wildcard external/lua/*.h)
INCLUDES+=$(wildcard external/miniaudio/*.h)
INCLUDES+=$(wildcard external/spleen/*.h)
INCLUDES+=$(wildcard external/stb/*.h)
INCLUDES+=$(wildcard external/xmp-lite/*.h)
INCLUDES+=$(wildcard external/noise/*.h)
INCLUDES+=$(wildcard external/chipmunk/*.h)
INCLUDES+=$(wildcard external/noise/*.h)
INCLUDES+=$(wildcard external/chipmunk/*.h)
#
SCRIPTS:=$(wildcard src/core/systems/vm/*.lua)
SCRIPTS+=$(wildcard src/core/systems/vm/modules/*.lua)
#
TEXTS:=$(wildcard src/assets/*.txt)
#
BEXTS:=$(wildcard src/assets/shaders/*.glsl)
#
PNGS:=$(wildcard src/assets/images/*.png)
PNGS+=$(wildcard external/spleen/*.png)
# Output files
OBJECTS:=$(SOURCES:%.c=%.o)
SDUMPS:=$(SCRIPTS:%.lua=%.inc)
TDUMPS:=$(TEXTS:%.txt=%.inc)
BDUMPS:=$(BEXTS:%.glsl=%.inc)
PDUMPS:=$(PNGS:%.png=%.inc)

default: $(TARGET)
all: default

$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LWARNINGS) $(LFLAGS) $(LOPTS) -o $@
	@echo "Linking complete!"

# The dependency upon `Makefile` is redundant, since scripts are bound to it.
$(OBJECTS): %.o : %.c $(SDUMPS) $(TDUMPS) $(BDUMPS) $(PDUMPS) $(INCLUDES) Makefile
	@$(COMPILER) $(CWARNINGS) $(CFLAGS) $(COPTS) -c $< -o $@
	@echo "Compiled '"$<"' successfully!"

# Define automatically rules to convert `.lua` script, `.txt` files, and `.rgba` images
# into an embeddable-ready `.inc` file. `.inc` files also depend upon `Makefile` to be
# rebuild in case of tweakings.
$(SDUMPS): %.inc: %.lua Makefile
	@$(ANALYZER) $(AFLAGS) $<
	@$(DUMPER) $(DFLAGS) $< > $@
	@echo "Generated '"$@"' from '"$<"' successfully!"

$(TDUMPS): %.inc : %.txt Makefile
	@$(DUMPER) $(DFLAGS) $< > $@
	@echo "Generated '"$@"' from '"$<"' successfully!"

$(BDUMPS): %.inc : %.glsl Makefile
	@$(DUMPER) $(DFLAGS) $< > $@
	@echo "Generated '"$@"' from '"$<"' successfully!"

$(PDUMPS): %.inc : %.png Makefile
	@convert $< RGBA:- | $(DUMPER) $(DFLAGS) > $@
	@echo "Generated '"$@"' from '"$<"' successfully!"

stats:
	@cloc ./src > stats.txt

primitives: $(TARGET)
	@echo "Launching *primitives* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/primitives
	./$(TARGET) --path=./demos/primitives

bunnymark: $(TARGET)
	@echo "Launching *bunnymark* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/bunnymark
	@./$(TARGET) --path=./demos/bunnymark

fire: $(TARGET)
	@echo "Launching *fire* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/fire
	@./$(TARGET) --path=./demos/fire

tiled-map: $(TARGET)
	@echo "Launching *tiled-map* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/tiled-map
	@./$(TARGET) --path=./demos/tiled-map

timers: $(TARGET)
	@echo "Launching *timers* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/timers
	@./$(TARGET) --path=./demos/timers

postfx: $(TARGET)
	@echo "Launching *postfx* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/postfx
	@./$(TARGET) --path=./demos/postfx

spritestack: $(TARGET)
	@echo "Launching *spritestack* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/spritestack
	@./$(TARGET) --path=./demos/spritestack

palette: $(TARGET)
	@echo "Launching *palette* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/palette
	@./$(TARGET) --path=./demos/palette

mode7: $(TARGET)
	@echo "Launching *mode7* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/mode7
	@./$(TARGET) --path=./demos/mode7

snake: $(TARGET)
	@echo "Launching *snake* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/snake
	@./$(TARGET) --path=./demos/snake

shades: $(TARGET)
	@echo "Launching *shades* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/shades
	@./$(TARGET) --path=./demos/shades

gamepad: $(TARGET)
	@echo "Launching *gamepad* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/gamepad
	@./$(TARGET) --path=./demos/gamepad

gamepad-pak: $(TARGET)
	@echo "Launching *gamepad (PAK)* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/gamepad
	@lua5.3 ./extras/pakgen.lua --input=./demos/gamepad --output=./demos/gamepad.pak --encrypted
	@./$(TARGET) --path=./demos/gamepad.pak

hello-tofu: $(TARGET)
	@echo "Launching *hello-tofu* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/hello-tofu
	@./$(TARGET) --path=./demos/hello-tofu

swirl: $(TARGET)
	@echo "Launching *swirl* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/swirl
	@./$(TARGET) --path=./demos/swirl

twist: $(TARGET)
	@echo "Launching *twist* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/twist
	@./$(TARGET) --path=./demos/twist

tween: $(TARGET)
	@echo "Launching *tween* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/tween
	@./$(TARGET) --path=./demos/tween

helix: $(TARGET)
	@echo "Launching *helix* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/helix
	@./$(TARGET) --path=./demos/helix

mixer: $(TARGET)
	@echo "Launching *mixer* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/mixer
	@./$(TARGET) --path=./demos/mixer

scaling: $(TARGET)
	@echo "Launching *scaling* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/scaling
	@./$(TARGET) --path=./demos/scaling

rotations: $(TARGET)
	@echo "Launching *rotations* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/rotations
	@./$(TARGET) --path=./demos/rotations

platform: $(TARGET)
	@echo "Launching *platform* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/platform
	@./$(TARGET) --path=./demos/platform

splash: $(TARGET)
	@echo "Launching *splash* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/splash
	@./$(TARGET) --path=./demos/splash

rasterbars: $(TARGET)
	@echo "Launching *rasterbars* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/rasterbars
	@./$(TARGET) --path=./demos/rasterbars

stencil: $(TARGET)
	@echo "Launching *stencil* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/stencil
	@./$(TARGET) --path=./demos/stencil

lasers: $(TARGET)
	@echo "Launching *lasers* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/lasers
	@./$(TARGET) --path=./demos/lasers

physics: $(TARGET)
	@echo "Launching *physics* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/physics
	@./$(TARGET) --path=./demos/physics

bump: $(TARGET)
	@echo "Launching *bump* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/bump
	@./$(TARGET) --path=./demos/bump

threedee: $(TARGET)
	@echo "Launching *threedee* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/threedee
	@./$(TARGET) --path=./demos/threedee

threedee-pak: $(TARGET)
	@echo "Launching *threedee (PAK)* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/threedee
	@lua5.3 ./extras/pakgen.lua --input=./demos/threedee --output=./demos/threedee.pak --encrypted
	@./$(TARGET) --path=./demos/threedee.pak

noise: $(TARGET)
	@echo "Launching *threedee* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/noise
	@./$(TARGET) --path=./demos/noise

demo: $(TARGET)
	@echo "Launching *$(DEMO)* application!"
	@$(ANALYZER) $(AFLAGS) ./demos/$(DEMO)
	@./$(TARGET) --path=./demos/$(DEMO)

valgrind: $(TARGET)
	@echo "Valgrind *$(DEMO)* application!"
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes env LIBGL_ALWAYS_SOFTWARE=1 ./$(TARGET) ./demos/$(DEMO)

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(SDUMPS)
	@$(RM) $(TDUMPS)
	@$(RM) $(PDUMPS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(RM) $(TARGET)
	@echo "Executable removed!"
