# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

FEATURE_CHARACTER_NAMES ?= 1
FEATURE_IDNA ?= 1
EXAMPLE_SOURCE = example.c

AMALGAMATION_DIR = ../build-amalgamation
AMALGAMATION_HEADER = $(AMALGAMATION_DIR)/mojibake.h
AMALGAMATION_SOURCE = $(AMALGAMATION_DIR)/mojibake.c

GENERATE_DIR = ../utils/generate

# Features
EXAMPLE_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES) \
	-DMJB_FEATURE_IDNA=$(FEATURE_IDNA)
NO_CHARACTER_NAMES_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=0 \
	-DMJB_FEATURE_IDNA=$(FEATURE_IDNA)
NO_IDNA_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES) \
	-DMJB_FEATURE_IDNA=0

# Basic example program without optional features
NO_FEATURES_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=0 -DMJB_FEATURE_IDNA=0

all: example example-no-character-names example-no-idna example-no-features

# Compile example program
example: $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)
	$(CC) $(CPPFLAGS) $(EXAMPLE_CPPFLAGS) $(CFLAGS) -o $@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without Unicode character name tables
example-no-character-names: $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)
	$(CC) $(CPPFLAGS) $(NO_CHARACTER_NAMES_CPPFLAGS) $(CFLAGS) -o $@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without IDNA support
example-no-idna: $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)
	$(CC) $(CPPFLAGS) $(NO_IDNA_CPPFLAGS) $(CFLAGS) -o $@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without optional features
example-no-features: $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)
	$(CC) $(CPPFLAGS) $(NO_FEATURES_CPPFLAGS) $(CFLAGS) -o $@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

$(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE):
	mkdir -p $(AMALGAMATION_DIR)
	cd $(GENERATE_DIR) && npm run generate -- amalgamation

clean:
	rm -f example example-no-character-names example-no-idna example-no-features

.PHONY: all example example-no-character-names example-no-idna example-no-features clean
