#
# MIT License
#
# Copyright (c) 2019-2023 Marco Lizza
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# This makefile has been written in accordance to the GNU general conventions
# available at the following URL
#
#   https://www.gnu.org/prep/standards/html_node/Makefile-Basics.html

# Directories are declared as lowercase.
builddir=./build
srcdir=./src
externaldir=./external
extrasdir=./extras
toolsdir=$(extrasdir)/tools
dockerdir=$(extrasdir)/docker

# The default target platform is Linux.
#
# Available values are: `linux`, `window`, and `raspberry-pi`.
ifeq ($(PLATFORM),)
	PLATFORM=linux
endif

# The default windowing system is X11.
#
# Available values are: `gdi`, `x11`, and `wayland`.
ifeq ($(WINDOWING),)
	ifeq ($(PLATFORM),windows)
		WINDOWING=gdi
	else ifeq ($(PLATFORM),linux)
		WINDOWING=x11
	else ifeq ($(PLATFORM),raspberry-pi)
		WINDOWING=x11
	endif
endif

# Define the target executable name, according to the current platform.
ifeq ($(PLATFORM),windows)
	ifeq ($(ARCHITECTURE),x64)
		TARGET=tofu_x64.exe
	else
		TARGET=tofu_x32.exe
	endif
else ifeq ($(PLATFORM),linux)
	TARGET=tofu
else ifeq ($(PLATFORM),raspberry-pi)
	TARGET=tofu_pi
else
$(error PLATFORM value '$(PLATFORM)' is not recognized)
endif

DOCKER=docker
DOCKER_IMAGE=tofu-build-env

KERNAL=kernal.pak

PACKER=$(toolsdir)/pakgen.lua
PACKERFLAGS=--encrypted --sorted --quiet

LUACHECK=luacheck
LUACHECKFLAGS=--no-self --std lua54 -q

VALGRIND=valgrind
VALGRINDFLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

# ------------------------------------------------------------------------------
# --- Compiler -----------------------------------------------------------------
# ------------------------------------------------------------------------------

ifeq ($(PLATFORM),windows)
	ifeq ($(ARCHITECTURE),x64)
		COMPILER=x86_64-w64-mingw32-gcc
	else
		COMPILER=i686-w64-mingw32-gcc
	endif
else
	COMPILER=gcc
endif

CWARNINGS=-std=c99 \
	-Wall \
	-Wextra \
	-Werror \
	-Winline \
	-Wlogical-op \
	-Wno-unused-parameter \
	-Wpedantic \
	-Wpointer-arith \
	-Wstrict-prototypes \
	-Wshadow \
	-Wunreachable-code \
	-Wwrite-strings
#	-Wfloat-equal

CFLAGS=-D_DEFAULT_SOURCE \
	-DLUA_32BITS -DLUA_FLOORN2I=F2Ifloor \
	-DSTB_IMAGE_WRITE_FLIP_VERTICALLY -DSTBI_ONLY_PNG -DSTBI_NO_STDIO \
	-DDR_FLAC_NO_STDIO \
	-DMA_NO_DECODING -DMA_NO_ENCODING -DMA_NO_GENERATION -DMA_NO_ENGINE -DMA_NO_RESOURCE_MANAGER \
	-DLIBXMP_BUILDING_STATIC \
	-DLUAX_NO_SYSTEM_LIBRARIES \
	-I$(srcdir) \
	-I$(externaldir)
ifeq ($(PLATFORM),linux)
	CFLAGS+=-DLUA_USE_LINUX
else ifeq ($(PLATFORM),raspberry-pi)
	CFLAGS+=-DLUA_USE_LINUX
endif
ifeq ($(WINDOWING),gdi)
	CFLAGS+=-D_GLFW_WIN32
else ifeq ($(WINDOWING),x11)
	CFLAGS+=-D_GLFW_X11
else ifeq ($(WINDOWING),wayland)
	CFLAGS+=-D_GLFW_WAYLAND
else
$(error WINDOWING value '$(WINDOWING)' is not recognized)
endif

ifeq ($(BUILD),release)
# -Ofast => -O3 -ffast-math
# -Os => -O2, favouring size
	COPTS=-O3 -ffast-math -DNDEBUG -fomit-frame-pointer
else ifeq ($(BUILD),profile)
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG -DPROFILE -pg
else ifeq ($(BUILD),sanitize-address)
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG -DSANITIZE -fsanitize=address -fno-omit-frame-pointer
else ifeq ($(BUILD),sanitize-leak)
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG -DSANITIZE -fsanitize=leak -fno-omit-frame-pointer
else
#	COPTS=-Og -ggdb3 -DDEBUG
#	COPTS=-O0 -ggdb3 -DDEBUG
	COPTS=-O0 -ffast-math -ggdb3 -DDEBUG
endif

# ------------------------------------------------------------------------------
# --- Linker -------------------------------------------------------------------
# ------------------------------------------------------------------------------

LINKER=$(COMPILER)

LFLAGS=-lpthread
ifeq ($(PLATFORM),windows)
	LFLAGS+=-lpsapi
else ifeq ($(PLATFORM),linux)
	LFLAGS+=-lm -ldl
else ifeq ($(PLATFORM),raspberry-pi)
	LFLAGS+=-lm -ldl -latomic
endif

ifeq ($(WINDOWING),gdi)
	LFLAGS+=-lgdi32
else ifeq ($(WINDOWING),x11)
	LFLAGS+=-lX11
else ifeq ($(WINDOWING),wayland)
	LFLAGS+=-lwayland-client -lwayland-server -lwayland-egl -lxkbcommon
endif

LWARNINGS=-Wall -Wextra -Werror

LOPTS=
ifeq ($(BUILD),release)
	LOPTS+=-fomit-frame-pointer
else ifeq ($(BUILD),profile)
	LOPTS+=-pg
else ifeq ($(BUILD),sanitize-address)
	LOPTS+=-fsanitize=address -fno-omit-frame-pointer
else ifeq ($(BUILD),sanitize-leak)
	LOPTS+=-fsanitize=leak -fno-omit-frame-pointer
endif

# ------------------------------------------------------------------------------
# --- Files --------------------------------------------------------------------
# ------------------------------------------------------------------------------

# Source files list (src)
SOURCES:=$(wildcard $(srcdir)/*.c) \
	$(wildcard $(srcdir)/core/*.c) \
	$(wildcard $(srcdir)/libs/*.c) \
	$(wildcard $(srcdir)/libs/fs/*.c) \
	$(wildcard $(srcdir)/libs/gl/*.c) \
	$(wildcard $(srcdir)/libs/sl/*.c) \
	$(wildcard $(srcdir)/modules/*.c) \
	$(wildcard $(srcdir)/modules/internal/*.c) \
	$(wildcard $(srcdir)/systems/*.c) \
	$(wildcard $(srcdir)/systems/storage/*.c)
# Source files list (external)
SOURCES+=$(wildcard $(externaldir)/dr_libs/*.c) \
	$(wildcard $(externaldir)/glad/*.c) \
	$(wildcard $(externaldir)/lua/*.c) \
	$(wildcard $(externaldir)/miniaudio/*.c) \
	$(wildcard $(externaldir)/spleen/*.c) \
	$(wildcard $(externaldir)/xmp-lite/*.c) \
	$(wildcard $(externaldir)/noise/*.c) \
	$(wildcard $(externaldir)/chipmunk/*.c)
# Include files list (src)
INCLUDES:=$(wildcard $(srcdir)/*.h) \
	$(wildcard $(srcdir)/core/*.h) \
	$(wildcard $(srcdir)/libs/*.h) \
	$(wildcard $(srcdir)/libs/fs/*.h) \
	$(wildcard $(srcdir)/libs/gl/*.h) \
	$(wildcard $(srcdir)/libs/sl/*.h) \
	$(wildcard $(srcdir)/modules/*.h) \
	$(wildcard $(srcdir)/modules/internal/*.h) \
	$(wildcard $(srcdir)/systems/*.h) \
	$(wildcard $(srcdir)/systems/storage/*.h)
# Include files list (external)
INCLUDES+=$(wildcard $(externaldir)/dr_libs/*.h) \
	$(wildcard $(externaldir)/glad/*.h) \
	$(wildcard $(externaldir)/lua/*.h) \
	$(wildcard $(externaldir)/miniaudio/*.h) \
	$(wildcard $(externaldir)/spleen/*.h) \
	$(wildcard $(externaldir)/stb/*.h) \
	$(wildcard $(externaldir)/xmp-lite/*.h) \
	$(wildcard $(externaldir)/noise/*.h) \
	$(wildcard $(externaldir)/chipmunk/*.h)

# Prepare GLFW flags according to the target platform.
depends_from = $(shell cat $(1) | sed 's|^|$(2)|g' | tr '\n' ' ')

SOURCES+=$(call depends_from,$(externaldir)/GLFW/dependencies/common_c.in,$(externaldir)/GLFW/)
INCLUDES+=$(call depends_from,$(externaldir)/GLFW/dependencies/common_h.in,$(externaldir)/GLFW/)
ifeq ($(WINDOWING),gdi)
	SOURCES+=$(call depends_from,$(externaldir)/GLFW/dependencies/win32_c.in,$(externaldir)/GLFW/)
	INCLUDES+=$(call depends_from,$(externaldir)/GLFW/dependencies/win32_h.in,$(externaldir)/GLFW/)
else ifeq ($(WINDOWING),x11)
	SOURCES+=$(call depends_from,$(externaldir)/GLFW/dependencies/x11_c.in,$(externaldir)/GLFW/)
	INCLUDES+=$(call depends_from,$(externaldir)/GLFW/dependencies/x11_h.in,$(externaldir)/GLFW/)
else ifeq ($(WINDOWING),wayland)
	SOURCES+=$(call depends_from,$(externaldir)/GLFW/dependencies/wayland_c.in,$(externaldir)/GLFW/)
	INCLUDES+=$(call depends_from,$(externaldir)/GLFW/dependencies/wayland_h.in,$(externaldir)/GLFW/)
else
	SOURCES+=$(call depends_from,$(externaldir)/GLFW/dependencies/osmesa_c.in,$(externaldir)/GLFW/)
	INCLUDES+=$(call depends_from,$(externaldir)/GLFW/dependencies/osmesa_h.in,$(externaldir)/GLFW/)
endif

# Add specific dependencies for Wayland build.
ifeq ($(WINDOWING),wayland)
	SOURCES+=$(wildcard $(externaldir)/wayland/*.c)
	INCLUDES+=$(wildcard $(externaldir)/wayland/*.h)
endif

#$(info SOURCES="$(SOURCES)")
#$(info INCLUDES="$(INCLUDES)")

# Output files
OBJECTS:=$(SOURCES:%.c=%.o)

# Everything in the `kernal` sub-folder will be packed into a seperate file.
RESOURCES:=$(shell find $(srcdir)/kernal -type f -name '*.*')

# Setting the docker-image dependencies.
DOCKER_FILES=$(dockerdir)/Dockerfile $(dockerdir)/docker_context

# ------------------------------------------------------------------------------
# --- Build Rules --------------------------------------------------------------
# ------------------------------------------------------------------------------

all: engine

# Docker develoment container. This enables to build the engine w/o tampering
# with the host (only docker is required). This could also be used to perform
# a CI build.
.PHONY: docker-create
docker-create: $(DOCKER_FILES)
	@$(DOCKER) build --force-rm -t $(DOCKER_IMAGE) -f $(DOCKER_FILES)

.PHONY: docker-launch
docker-launch: $(DOCKER_FILES)
	@$(DOCKER) run --rm -t -i -e USER_ID=$(shell id -u) -e GROUP_ID=$(shell id -g) -v $(shell pwd):/tofu tofu-build-env

.PHONY: docker-clean
docker-clean: $(DOCKER_FILES)
	@$(DOCKER) image rm -f `$(DOCKER) image ls | grep $(DOCKER_IMAGE) | awk '{print $$3}'`

.PHONY: check
check:
	@find $(srcdir)/kernal -name '*.lua' | xargs $(LUACHECK) $(LUACHECKFLAGS)
	@cppcheck --force --enable=all $(srcdir) > /dev/null
	@echo "Checking complete!"

# Geneartes the code statistics for the project. Ideally this should be
# performed right before a new release, and the output included in the release
# data.
.PHONY: stats
stats:
	@cloc $(srcdir) > $(builddir)/stats.txt

engine: $(builddir) $(builddir)/$(TARGET) $(builddir)/$(KERNAL)

$(builddir):
	mkdir -p $(builddir)

# The `kernal.pak` archive contains all the Lua scripts that constitue (part of)
# and runtime. It's required for the engine to work and it's repacked each time
# a file is changed.
#
# In case we want to embed pre-compiled scripts
#
#	@luac5.4 $< -o $@

$(builddir)/$(KERNAL): $(RESOURCES) Makefile
	@find $(srcdir)/kernal -name '*.lua' | xargs $(LUACHECK) $(LUACHECKFLAGS)
	@$(PACKER) $(PACKERFLAGS) $(srcdir)/kernal --output=$(builddir)/$(KERNAL)
	@echo "Kernal packed!"

$(builddir)/$(TARGET): $(OBJECTS) Makefile
	@$(LINKER) $(OBJECTS) $(LOPTS) $(LWARNINGS) $(LFLAGS) -o $@
	@echo "Linking complete!"

# The dependency upon `Makefile` is redundant, since scripts are bound to it.
$(OBJECTS): %.o : %.c $(DUMPS) $(INCLUDES) Makefile
	@$(COMPILER) $(COPTS) $(CWARNINGS) $(CFLAGS) -c $< -o $@
	@echo "Compiled '"$<"' successfully!"

.PHONY: clean
clean:
	@rm -f $(OBJECTS)
	@rm -f $(DUMPS)
	@rm -f $(builddir)/$(KERNAL)
	@rm -f $(builddir)/$(TARGET)
	@echo "Cleanup complete!"

.PHONY: clean-all
clean-all:
	@make docker-clean
	@make clean
	@echo "Deep cleanup complete!"

# Demos
primitives: engine
	@echo "Launching *primitives* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/primitives
	@$(builddir)/$(TARGET) --data=./demos/primitives

bunnymark: engine
	@echo "Launching *bunnymark* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/bunnymark
	@$(builddir)/$(TARGET) --data=./demos/bunnymark

fire: engine
	@echo "Launching *fire* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/fire
	@$(builddir)/$(TARGET) --data=./demos/fire

tiled-map: engine
	@echo "Launching *tiled-map* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/tiled-map
	@$(builddir)/$(TARGET) --data=./demos/tiled-map

timers: engine
	@echo "Launching *timers* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/timers
	@$(builddir)/$(TARGET) --data=./demos/timers

postfx: engine
	@echo "Launching *postfx* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/postfx
	@$(builddir)/$(TARGET) --data=./demos/postfx

spritestack: engine
	@echo "Launching *spritestack* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/spritestack
	@$(builddir)/$(TARGET) --data=./demos/spritestack

palette: engine
	@echo "Launching *palette* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/palette
	@$(builddir)/$(TARGET) --data=./demos/palette

mode7: engine
	@echo "Launching *mode7* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/mode7
	@$(builddir)/$(TARGET) --data=./demos/mode7

snake: engine
	@echo "Launching *snake* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/snake
	@$(builddir)/$(TARGET) --data=./demos/snake

shades: engine
	@echo "Launching *shades* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/shades
	@$(builddir)/$(TARGET) --data=./demos/shades

gamepad: engine
	@echo "Launching *gamepad* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/gamepad
	@$(builddir)/$(TARGET) --data=./demos/gamepad

gamepad-pak: engine
	@echo "Launching *gamepad (PAK)* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/gamepad
	@$(PACKER) $(PACKERFLAGS) ./demos/gamepad --output=./demos/gamepad.pak
	@$(builddir)/$(TARGET) --data=./demos/gamepad.pak

hello-tofu: engine
	@echo "Launching *hello-tofu* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/hello-tofu
	@$(builddir)/$(TARGET) --data=./demos/hello-tofu

swirl: engine
	@echo "Launching *swirl* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/swirl
	@$(builddir)/$(TARGET) --data=./demos/swirl

twist: engine
	@echo "Launching *twist* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/twist
	@$(builddir)/$(TARGET) --data=./demos/twist

tween: engine
	@echo "Launching *tween* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/tween
	@$(builddir)/$(TARGET) --data=./demos/tween

helix: engine
	@echo "Launching *helix* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/helix
	@$(builddir)/$(TARGET) --data=./demos/helix

mixer: engine
	@echo "Launching *mixer* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/mixer
	@$(builddir)/$(TARGET) --data=./demos/mixer

scaling: engine
	@echo "Launching *scaling* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/scaling
	@$(builddir)/$(TARGET) --data=./demos/scaling

rotations: engine
	@echo "Launching *rotations* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/rotations
	@$(builddir)/$(TARGET) --data=./demos/rotations

platform: engine
	@echo "Launching *platform* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/platform
	@$(builddir)/$(TARGET) --data=./demos/platform

splash: engine
	@echo "Launching *splash* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/splash
	@$(builddir)/$(TARGET) --data=./demos/splash

rasterbars: engine
	@echo "Launching *rasterbars* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/rasterbars
	@$(builddir)/$(TARGET) --data=./demos/rasterbars

stencil: engine
	@echo "Launching *stencil* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/stencil
	@$(builddir)/$(TARGET) --data=./demos/stencil

lasers: engine
	@echo "Launching *lasers* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/lasers
	@$(builddir)/$(TARGET) --data=./demos/lasers

physics: engine
	@echo "Launching *physics* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/physics
	@$(builddir)/$(TARGET) --data=./demos/physics

bump: engine
	@echo "Launching *bump* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/bump
	@$(builddir)/$(TARGET) --data=./demos/bump

threedee: engine
	@echo "Launching *threedee* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/threedee
	@$(builddir)/$(TARGET) --data=./demos/threedee

threedee-pak: engine
	@echo "Launching *threedee (PAK)* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/threedee
	@$(PACKER) $(PACKERFLAGS) ./demos/threedee --output=./demos/threedee.pak
	@$(builddir)/$(TARGET) --data=./demos/threedee.pak

noise: engine
	@echo "Launching *noise* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/noise
	@$(builddir)/$(TARGET) --data=./demos/noise

copperbars: engine
	@echo "Launching *copperbars* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/copperbars
	@$(builddir)/$(TARGET) --data=./demos/copperbars

scroller: engine
	@echo "Launching *scroller* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/scroller
	@$(builddir)/$(TARGET) --data=./demos/scroller

cellular: engine
	@echo "Launching *cellular* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/cellular
	@$(builddir)/$(TARGET) --data=./demos/cellular

demo: engine
	@echo "Launching *$(DEMO)* application!"
	@$(LUACHECK) $(LUACHECKFLAGS) ./demos/$(DEMO)
	@$(builddir)/$(TARGET) --data=./demos/$(DEMO)

# Use software renderer to use VALGRIND
valgrind: engine
	@echo "Valgrind *$(DEMO)* application!"
	@export LIBGL_ALWAYS_SOFTWARE=1
	@(VALGRIND) $(VALGRINDFLAGS) $(builddir)/$(TARGET) --data=./demos/$(DEMO)
	@export LIBGL_ALWAYS_SOFTWARE=0
