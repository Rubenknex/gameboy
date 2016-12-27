SOURCES = $(wildcard *.cpp)
OBJS = $(SOURCES:.cpp=.o)
CC = g++
COMP_FLAGS = -w -g
LINK_FLAGS = -lSDL2

all: gameboy

gameboy: $(OBJS)
	$(CC) $(OBJS) $(COMP_FLAGS) $(LINK_FLAGS) -o gameboy

.cpp.o:
	$(CC) $(COMP_FLAGS) $(LINK_FLAGS) -c $<

clean:
	rm *o