
MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

ASAN ?= 0
NO_OPTIMIZE ?= 0

CXX = clang++

CXXFLAGS = -g -fdiagnostics-color=always -std=c++23 -Wall -Werror -Wpedantic -Wextra
CXXFLAGS += -Wno-gnu-empty-struct, -Wno-zero-length-array, -Wno-c99-extensions, -Wno-flexible-array-extensions, -Wno-vla-cxx-extension

ifneq ($(NO_OPTIMIZE), 1)
    CXXFLAGS += -O3 -march=native -mtune=native -flto -fvisibility=hidden -fvisibility-inlines-hidden -fno-stack-protector -fomit-frame-pointer
endif

ifeq ($(ASAN), 1)
    CXXFLAGS += -fsanitize=address
endif

SRC = cpp/index.cpp
TARGET = cpp/.vls_compiled
LDFLAGS = -lm

TRACKED_FILES = $(shell find $(MAKEFILE_DIR) -type f)

BUILD_COMMAND_FILE = src/vls/.build_command
BUILD_COMMAND = $(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

ifneq ($(shell cat $(BUILD_COMMAND_FILE) 2>/dev/null), $(BUILD_COMMAND))
$(shell echo '$(BUILD_COMMAND)' > $(BUILD_COMMAND_FILE))
endif

main: $(TARGET)

$(TARGET): $(TRACKED_FILES) $(BUILD_COMMAND_FILE)
	$(BUILD_COMMAND)
