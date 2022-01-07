# All .cpp files
SRCS = $(wildcard *.cpp)
OBJDIR = build
OBJS := $(SRCS:%.cpp=$(OBJDIR)/%.o)

CC = g++
# -Wall: show all warnings, -g: include debugging symbols
COMP_FLAGS = -Wall -g
LINK_FLAGS = -lSDL2 -lSDL2_ttf

# Compile .cpp into .o file
$(OBJDIR)/%.o: %.cpp
	@echo "Compiling $< into $@"
	$(CC) -c $(COMP_FLAGS) $(LINK_FLAGS) $< -o $@

# Link .o files into executable file
gameboy: $(OBJS)
	$(CC) $(OBJS) $(COMP_FLAGS) $(LINK_FLAGS) -o gameboy

clean:
	rm *o