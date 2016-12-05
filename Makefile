
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

#LDFLAGS="-Wl,-rpath,../libs/"
LFLAGS=
LIBS=-lsimlib

INT := $(shell command -v install_name_tool 2> /dev/null)

# rule-expression: dependency-expression
# $^ => dependency-expression
# $@ => rule-expression

# Default action
all: $(EXECUTABLE)

# Compile without .o files (create only executable)
$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(dir $@)
	@echo "Executable: $(notdir $@)"
	@$(LINK.o) $(CFLAGS) $^ -o $@ $(LFLAGS) $(LIBS)
ifdef INT
	@install_name_tool -change simlib.so /usr/local/lib/libsimlib.so $(EXECUTABLE) || true
endif
	
#@cp "/usr/local/lib/libsimlib.so" "./simlib.so"
	
%.o: %.cpp
	@echo "Compile: $<  => $(notdir $@)"
	@$(CC) $(CFLAGS) -c $<  -o $@
	
# Clean executable
clean:
	@rm -f simlib.so
	@rm -f $(EXECUTABLE)
	@rm -f *.o

# Usage: ./restaurace [<settlers(1)> <waiters(3)> <tables(24)> <cookers(6)> <ct_ratio(1.0)> pf_ratio(1.0)]
run:
	@./$(EXECUTABLE) 1 3 24 6 1.0 1.0 >"experiment0.txt"
	@./$(EXECUTABLE) 1 3 48 6 0.5 1.0 >"experiment1.txt"
	@./$(EXECUTABLE) 1 1 24 6 1.0 1.0 >"experiment2.txt"
	@./$(EXECUTABLE) 1 3 24 6 1.0 2.0 >"experiment3.txt"
	@./$(EXECUTABLE) 1 1 24 3 2.0 1.0 >"experiment4.txt"
	@./$(EXECUTABLE)
	

