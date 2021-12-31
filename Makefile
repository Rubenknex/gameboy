# All .cpp files
SOURCES = $(wildcard *.cpp)

OBJS = $(SOURCES:.cpp=.o)
CC = g++
# -Wall: show all warnings, -g: include debugging symbols
COMP_FLAGS = -Wall -g
LINK_FLAGS = -lSDL2

all: gameboy

# Link .o files into executable file
gameboy: $(OBJS)
	$(CC) $(OBJS) $(COMP_FLAGS) $(LINK_FLAGS) -o gameboy

# Compile .cpp into .o file
.cpp.o:
	$(CC) $(COMP_FLAGS) $(LINK_FLAGS) -c $<

clean:
	rm *o