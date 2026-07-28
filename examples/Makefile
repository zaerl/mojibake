# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

CPPFLAGS ?= -DNDEBUG
CFLAGS ?= -O3
CXXFLAGS ?= -O3
RUSTFLAGS ?= -C opt-level=3
RUSTC ?= rustc
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

# C++ files
CPP_EXAMPLE_SOURCE = cpp/example.cpp
CPP_WRAPPER = ../src/cpp/mojibake.hpp
CPP_EXAMPLE = $(EXAMPLE_BUILD_DIR)/example-cpp
CPP_EXAMPLE_DEPS = $(CPP_EXAMPLE_SOURCE) $(CPP_WRAPPER) $(AMALGAMATION_HEADER) \
	$(AMALGAMATION_SOURCE)

# Python files
PYTHON_EXAMPLE_SOURCE = python/example.py

ifeq ($(OS),Windows_NT)
PYTHON_LIBRARY = $(EXAMPLE_BUILD_DIR)/mojibake.dll
PYTHON_SHARED_FLAGS = -shared
else ifeq ($(shell uname -s),Darwin)
PYTHON_LIBRARY = $(EXAMPLE_BUILD_DIR)/libmojibake.dylib
PYTHON_SHARED_FLAGS = -dynamiclib
else
PYTHON_LIBRARY = $(EXAMPLE_BUILD_DIR)/libmojibake.so
PYTHON_SHARED_FLAGS = -shared -fPIC
endif

# Rust files
RUST_EXAMPLE_SOURCE = rust/example.rs
RUST_LIBRARY_OBJECT = $(EXAMPLE_BUILD_DIR)/mojibake-rust.o
RUST_LIBRARY = $(EXAMPLE_BUILD_DIR)/libmojibake-rust.a
RUST_EXAMPLE = $(EXAMPLE_BUILD_DIR)/example-rust
RUST_EXAMPLE_DEPS = $(RUST_EXAMPLE_SOURCE) $(RUST_LIBRARY)

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

# Compile the C++ example program
example-cpp: $(CPP_EXAMPLE_DEPS) | $(EXAMPLE_BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(EXAMPLE_CPPFLAGS) $(CXXFLAGS) -std=c++17 -x c++ \
		-I $(AMALGAMATION_DIR) -o $(CPP_EXAMPLE) $(CPP_EXAMPLE_SOURCE) \
		$(AMALGAMATION_SOURCE)

# Build the shared library used by the Python example
example-python: $(PYTHON_EXAMPLE_SOURCE) $(PYTHON_LIBRARY)

$(PYTHON_LIBRARY): $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(EXAMPLE_CPPFLAGS) $(CFLAGS) -DMJB_SHARED=1 \
		-DMJB_BUILDING_LIBRARY=1 $(PYTHON_SHARED_FLAGS) -o $@ $(AMALGAMATION_SOURCE)

# Compile the Rust example program
example-rust: $(RUST_EXAMPLE_DEPS)
	$(RUSTC) $(RUSTFLAGS) --edition=2021 -L native=$(EXAMPLE_BUILD_DIR) \
		-l static=mojibake-rust -o $(RUST_EXAMPLE) $(RUST_EXAMPLE_SOURCE)

# Build the static library used by the Rust example
$(RUST_LIBRARY): $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE) | $(EXAMPLE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(EXAMPLE_CPPFLAGS) $(CFLAGS) -c \
		-o $(RUST_LIBRARY_OBJECT) $(AMALGAMATION_SOURCE)
	$(AR) rcs $@ $(RUST_LIBRARY_OBJECT)

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

.PHONY: all example example-cpp example-python example-rust example-zig \
	example-no-character-names example-no-collation example-no-idna \
	example-no-security example-no-features clean
