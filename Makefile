# project name (generate executable with this name)
TARGET=zest

COMPILER=g++
CFLAGS=-g -Wall -Wextra -Iexternal/include -DCHAISCRIPT_NO_THREADS

LINKER=g++
LFLAGS=-Wall -Wextra -Lexternal/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# change these to proper directories where each file should be
SRCDIR=src
OBJDIR=obj
BINDIR=bin

SOURCES:= $(wildcard $(SRCDIR)/*.cpp)
INCLUDES:= $(wildcard $(SRCDIR)/*.hpp)
OBJECTS:= $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
rm=rm -f

default: $(BINDIR)/$(TARGET)
all: default

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(COMPILER) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"