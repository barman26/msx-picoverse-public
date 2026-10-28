######################################################################
# MSX PICOVERSE PROJECT
# (c) 2026 Cristiano Goncalves
# The Retro Hacker
#
# Makefile - build script for the MSX PICOVERSE 2350 Yamanooto emulation
#
# Builds both the RP2350 firmware and the PC tool.
######################################################################

# Shared version (can be overridden on the command line)
VERSION ?= v1.19

# Paths
PICO_DIR := pico/yamanooto
PICO_BUILD := $(PICO_DIR)/build
PICO_CONFIG := $(PICO_BUILD)/CMakeCache.txt
TOOL_DIR := tool

# CMake options
CMAKE ?= c:/Users/Cristiano/.pico-sdk/cmake/v3.29.9/bin/cmake
# Use Ninja on Windows to avoid NMake and missing MinGW make.
ifeq ($(OS),Windows_NT)
	CMAKE_GENERATOR ?= Ninja
	CMAKE_MAKE_PROGRAM ?= C:/Users/Cristiano/.pico-sdk/ninja/v1.12.1/ninja.exe
else
	CMAKE_GENERATOR ?=
endif
CMAKE_GENERATOR_FLAG := $(if $(CMAKE_GENERATOR),-G "$(CMAKE_GENERATOR)",)
CMAKE_BUILD_TYPE ?= Release
CMAKE_MAKE_PROGRAM_FLAG := $(if $(CMAKE_MAKE_PROGRAM),-DCMAKE_MAKE_PROGRAM="$(CMAKE_MAKE_PROGRAM)",)

# Make options
MAKE ?= make

.PHONY: all firmware tool clean

all: firmware tool

firmware: $(PICO_CONFIG)
	@echo "Building Pico firmware"
	@$(CMAKE) --build "$(PICO_BUILD)"

$(PICO_CONFIG):
	@$(CMAKE) $(CMAKE_GENERATOR_FLAG) $(CMAKE_MAKE_PROGRAM_FLAG) -S "$(PICO_DIR)" -B "$(PICO_BUILD)" -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

tool: firmware
	@echo "Building PC tool"
	@"$(MAKE)" -C "$(TOOL_DIR)" VERSION=$(VERSION)

clean:
	@echo "Cleaning firmware and tool"
	@"$(MAKE)" -C "$(TOOL_DIR)" clean VERSION=$(VERSION)
	@$(CMAKE) --build "$(PICO_BUILD)" --target clean || true
	@$(CMAKE) -E rm -rf "$(PICO_BUILD)"
