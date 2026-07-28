# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

CPPFLAGS ?= -DNDEBUG
CFLAGS ?= -O3
ZIG ?= zig

FEATURE_CHARACTER_NAMES ?= 1
FEATURE_COLLATION ?= 1
FEATURE_IDNA ?= 1
FEATURE_SECURITY ?= 1
EXAMPLE_BUILD_DIR = build

# Amalgamation files
AMALGAMATION_DIR = ../build-amalgamation
AMALGAMATION_HEADER = $(AMALGAMATION_DIR)/mojibake.h
AMALGAMATION_SOURCE = $(AMALGAMATION_DIR)/mojibake.c

# C files
EXAMPLE_SOURCE = example.c

# Zig files
ZIG_EXAMPLE_SOURCE = zig/example.zig
ZIG_BINDINGS = $(EXAMPLE_BUILD_DIR)/mojibake.zig
ZIG_EXAMPLE = $(EXAMPLE_BUILD_DIR)/example-zig
ZIG_EXAMPLE_DEPS = $(ZIG_EXAMPLE_SOURCE) $(ZIG_BINDINGS) $(AMALGAMATION_SOURCE)

GENERATE_DIR = ../utils/generate
EXAMPLE_DEPS = $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)

# Features
EXAMPLE_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES) \
	-DMJB_FEATURE_COLLATION=$(FEATURE_COLLATION) \
	-DMJB_FEATURE_IDNA=$(FEATURE_IDNA) \
	-DMJB_FEATURE_SECURITY=$(FEATURE_SECURITY)
NO_CHARACTER_NAMES_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=0 \
	-DMJB_FEATURE_COLLATION=$(FEATURE_COLLATION) \
	-DMJB_FEATURE_IDNA=$(FEATURE_IDNA) \
	-DMJB_FEATURE_SECURITY=$(FEATURE_SECURITY)
NO_COLLATION_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES) \
	-DMJB_FEATURE_COLLATION=0 \
	-DMJB_FEATURE_IDNA=$(FEATURE_IDNA) \
	-DMJB_FEATURE_SECURITY=$(FEATURE_SECURITY)
NO_IDNA_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES) \
	-DMJB_FEATURE_COLLATION=$(FEATURE_COLLATION) \
	-DMJB_FEATURE_IDNA=0 \
	-DMJB_FEATURE_SECURITY=$(FEATURE_SECURITY)
NO_SECURITY_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES) \
	-DMJB_FEATURE_COLLATION=$(FEATURE_COLLATION) \
	-DMJB_FEATURE_IDNA=$(FEATURE_IDNA) \
	-DMJB_FEATURE_SECURITY=0

# Basic example program without optional features
NO_FEATURES_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=0 -DMJB_FEATURE_COLLATION=0 \
	-DMJB_FEATURE_IDNA=0 -DMJB_FEATURE_SECURITY=0

all: example example-no-character-names example-no-collation example-no-idna example-no-security \
	example-no-features

# Compile example program
example: $(EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(EXAMPLE_CPPFLAGS) $(CFLAGS) -o $(EXAMPLE_BUILD_DIR)/$@ \
		$(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile the Zig example program
example-zig: $(ZIG_EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(ZIG) build-exe --dep mojibake -I $(AMALGAMATION_DIR) $(AMALGAMATION_SOURCE) \
		-Mroot=$(ZIG_EXAMPLE_SOURCE) -Mmojibake=$(ZIG_BINDINGS) \
		-lc -femit-bin=$(ZIG_EXAMPLE)

# Generate Zig bindings from the amalgamation header
$(ZIG_BINDINGS): $(AMALGAMATION_HEADER) | $(EXAMPLE_BUILD_DIR)
	$(ZIG) translate-c $(AMALGAMATION_HEADER) -I $(AMALGAMATION_DIR) > $@.tmp
	mv $@.tmp $@

# Compile example program without Unicode character name tables
example-no-character-names: $(EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(NO_CHARACTER_NAMES_CPPFLAGS) $(CFLAGS) \
		-o $(EXAMPLE_BUILD_DIR)/$@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without Unicode collation support
example-no-collation: $(EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(NO_COLLATION_CPPFLAGS) $(CFLAGS) -o $(EXAMPLE_BUILD_DIR)/$@ \
		$(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without IDNA support
example-no-idna: $(EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(NO_IDNA_CPPFLAGS) $(CFLAGS) -o $(EXAMPLE_BUILD_DIR)/$@ \
		$(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without Unicode security support
example-no-security: $(EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(NO_SECURITY_CPPFLAGS) $(CFLAGS) -o $(EXAMPLE_BUILD_DIR)/$@ \
		$(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without optional features
example-no-features: $(EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(NO_FEATURES_CPPFLAGS) $(CFLAGS) -o $(EXAMPLE_BUILD_DIR)/$@ \
		$(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

$(EXAMPLE_BUILD_DIR):
	mkdir -p $@

$(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE):
	mkdir -p $(AMALGAMATION_DIR)
	cd $(GENERATE_DIR) && npm run generate -- amalgamation

clean:
	rm -rf $(EXAMPLE_BUILD_DIR)
	rm -f zig/mojibake.zig zig/mojibake.zig.tmp

.PHONY: all example example-zig example-no-character-names example-no-collation \
	example-no-idna example-no-security example-no-features clean
