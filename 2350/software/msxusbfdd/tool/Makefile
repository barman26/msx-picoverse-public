######################################################################
# MSX PICOVERSE PROJECT                                               
# (c) 2026 Cristiano Goncalves                                        
# The Retro Hacker                                                    
#                                                                      
# Makefile - build script for the MSX PICOVERSE 2350 USB FDD tool    
#                                                                      
# This Makefile compiles the msxusbfdd utility, regenerates the embedded 
# Raspberry Pi Pico firmware and MSX ROM headers, and packages the
# executable.      
######################################################################

# Toolchain configuration
CC      := gcc
XXD     := xxd

# Directory layout
SRCDIR  := src
BINDIR  := build
DISDIR  := dist
UTLDIR  := utils

# Build flags
VERBOSE ?= --verbose

# Debug control: set DEBUG=1 when invoking make to enable debug symbols and -DDEBUG
# Example: `make DEBUG=1`
DEBUG ?= 0

ifeq ($(DEBUG),1)
CCFLAGS := -g -DDEBUG
else
CCFLAGS := -g
endif

# Application metadata
VERSION ?= v1.01

# Project files
SOURCES := msxusbfdd.c
OUTFILE := msxusbfdd.exe

# Firmware assets (generated headers embed the Pico firmware image and the MSX BIOS ROM)
PICOBIN   := ../pico/msxusbfdd/dist/msxusbfdd.bin
PICOBIN_H := $(SRCDIR)/msxusbfdd.h
MSXROM    := ../msx/dist/msxusbfdd.rom
MSXROM_H  := $(SRCDIR)/msxusbfdd_rom.h

# Helpers
RM := rm -f

.PHONY: all compile package clean

all: clean compile package

compile: $(BINDIR)/$(OUTFILE)

package: $(DISDIR)/msxusbfdd.exe

$(BINDIR)/$(OUTFILE): $(BINDIR) $(SRCDIR)/$(SOURCES) $(PICOBIN_H) $(PICOBIN) $(MSXROM_H) $(MSXROM)
	@echo "Compiling $@"
	$(CC) $(CCFLAGS) -DAPP_VERSION=\"$(VERSION)\" $(SRCDIR)/$(SOURCES) -o $@

$(PICOBIN_H): $(PICOBIN)
	@echo "Embedding Pico firmware into header"
	$(UTLDIR)/$(XXD) -i $< $@

$(MSXROM_H): $(MSXROM)
	@echo "Embedding MSX USB FDD BIOS ROM into header"
	$(UTLDIR)/$(XXD) -i $< $@

$(BINDIR):
	@mkdir $@

$(DISDIR):
	@mkdir $@

$(DISDIR)/msxusbfdd.exe: $(DISDIR) $(BINDIR)/$(OUTFILE)
	@echo "Packaging $@"
	cp $(BINDIR)/$(OUTFILE) $@

clean:
	@echo "Cleaning ...."
	$(RM) $(BINDIR)/*.exe $(BINDIR)/*.uf2 $(SRCDIR)/msxusbfdd.h $(SRCDIR)/msxusbfdd_rom.h
	$(RM) $(DISDIR)/*
