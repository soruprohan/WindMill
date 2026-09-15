# Windmill Farm - alternative build for the MSYS2 MINGW64 shell.
# (In VS Code use build.ps1 / the Build task instead.)
#
#   make          build
#   make run      build and run from the project root
#   make clean    remove build/

CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -Wall -Iinclude -Ilibs
CFLAGS   := -Iinclude
LDLIBS   := -lglfw3 -lopengl32 -lgdi32

SRC_DIR   := src
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/WindmillFarm.exe

CPP_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
C_SRCS   := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRCS)) \
            $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
HEADERS  := $(wildcard $(SRC_DIR)/*.h)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Run from the project root so shaders/ and textures/ resolve.
run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
