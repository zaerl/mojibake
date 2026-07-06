# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

FEATURE_CHARACTER_NAMES ?= 1
EXAMPLE_SOURCE = example.c

AMALGAMATION_DIR = ../build-amalgamation
AMALGAMATION_HEADER = $(AMALGAMATION_DIR)/mojibake.h
AMALGAMATION_SOURCE = $(AMALGAMATION_DIR)/mojibake.c

GENERATE_DIR = ../utils/generate
EXAMPLE_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=$(FEATURE_CHARACTER_NAMES)
NO_CHARACTER_NAMES_CPPFLAGS = -DMJB_FEATURE_CHARACTER_NAMES=0

all: example

# Compile example program
example: $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)
	$(CC) $(CPPFLAGS) $(EXAMPLE_CPPFLAGS) $(CFLAGS) -o $@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

# Compile example program without Unicode character name tables
example-no-character-names: $(EXAMPLE_SOURCE) $(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE)
	$(CC) $(CPPFLAGS) $(NO_CHARACTER_NAMES_CPPFLAGS) $(CFLAGS) -o $@ $(EXAMPLE_SOURCE) $(AMALGAMATION_SOURCE)

$(AMALGAMATION_HEADER) $(AMALGAMATION_SOURCE):
	mkdir -p $(AMALGAMATION_DIR)
	cd $(GENERATE_DIR) && npm run generate -- amalgamation

clean:
	rm -f example example-no-character-names

.PHONY: all example example-no-character-names clean
