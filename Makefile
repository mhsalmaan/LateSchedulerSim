# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pthread -Iinclude -O2

# Directories
SRC_DIR := src
INC_DIR := include
BIN_DIR := bin
RES_DIR := results
DATA_DIR := data

# Source files and output binary
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
TARGET := $(BIN_DIR)/late

# Default target
all: setup $(TARGET)

# Create bin and results directories if they don't exist
setup:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(RES_DIR)

# Link object files to create the binary
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Clean build artifacts
clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(TARGET)
	rm -rf $(RES_DIR)/*

# Run the binary with a sample test (adjust arguments as needed)
run: all
	$(TARGET) $(DATA_DIR)/input.txt 2 4

.PHONY: all clean setup run
