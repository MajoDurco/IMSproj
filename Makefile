
# Compiler
CC=g++
# Compiler options
CFLAGS=-std=c++11
# Executable filename 
EXECUTABLE=restaurace

# Get source file names
SOURCES=$(wildcard *.cpp)

# Generate object names from sources
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))

LFLAGS=
LIBS=-lsimlib

# rule-expression: dependency-expression
# $^ => dependency-expression
# $@ => rule-expression

# Default action
all: $(EXECUTABLE)

# Compile without .o files (create only executable)
$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Executable: $(notdir $@)"
	@$(LINK.o) $(CFLAGS) $^ -o $@ $(LIBS)
	@cp "/usr/local/lib/libsimlib.so" "./simlib.so"
	
%.o: %.cpp
	@echo "Compile: $<  => $(notdir $@)"
	@$(CC) $(CFLAGS) -c $<  -o $@ $(LFLAGS)
	
# Clean executable
clean:
	@rm -f simlib.so
	@rm -f $(EXECUTABLE)
	@rm -f *.o
	
run:
	@./$(EXECUTABLE)
	

