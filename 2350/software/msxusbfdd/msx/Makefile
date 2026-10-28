######################################################################
# MSX PICOVERSE PROJECT
# (c) 2026 Cristiano Goncalves
# The Retro Hacker
#
# Makefile - build script for the PicoVerse 2040 USB FDD BIOS ROM
#
# Assembles the 32KB MSX ROM (two 16KB banks) with sjasm. The ROM is
# embedded in the UF2 file by the PC tool.
######################################################################

SJASM = tools/sjasm

SRCDIR = src
BINDIR = build
DISDIR = dist

# Project metadata (shared with the top-level Makefile)
VERSION ?= v1.01

OUTFILE = msxusbfdd.rom
SOURCES = $(wildcard $(SRCDIR)/*.asm) $(wildcard $(SRCDIR)/bank0/*.asm) $(wildcard $(SRCDIR)/bank1/*.asm)

.PHONY: all compile package clean FORCE

all: clean compile package

compile: $(BINDIR)/$(OUTFILE)

# The version string is included by bank1/verbose_reset.asm (boot banner)
$(BINDIR)/version.asm: FORCE
	@mkdir -p $(BINDIR)
	@echo '    db "$(VERSION)"' > $@

$(BINDIR)/$(OUTFILE): $(SOURCES) $(BINDIR)/version.asm
	@echo "Assembling $@"
	cd $(SRCDIR) && ../$(SJASM) msxusbfdd.asm ../$@

package:
	@echo "Packaging..."
	@mkdir -p $(DISDIR)
	cp $(BINDIR)/$(OUTFILE) $(DISDIR)/$(OUTFILE)

clean:
	@echo "Cleaning ...."
	rm -f $(BINDIR)/$(OUTFILE) $(BINDIR)/version.asm $(BINDIR)/*.lst
