
# Compiler
CC=g++
# Compiler options
CFLAGS=
# Executable filename 
EXECUTABLE=restaurace

# Get source file names
SOURCES=$(wildcard *.cpp)

# Generate object names from sources
OBJECTS=$(patsubst %.cpp,%.o,$(SOURCES))

LFLAGS=
LIBS=

# rule-expression: dependency-expression
# $^ => dependency-expression
# $@ => rule-expression

# Default action
all: $(EXECUTABLE)

# Compile without .o files (create only executable)
$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Executable: $(notdir $@)"
	@$(LINK.o) $(CFLAGS) $^ -o $@
	
%.o: %.cpp
	@echo "Compile: $<  => $(notdir $@)"
	@$(CC) $(CFLAGS) -c $<  -o $@ $(LFLAGS) $(LIBS)
	
# Clean executable
clean:
	@rm -f $(EXECUTABLE)
	@rm -f *.o
	
run:
	@./$(EXECUTABLE)
	

