# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

all: example

# Compile example program
example: example.c ../build-amalgamation/mojibake.c
	$(CC) -o example example.c ../build-amalgamation/mojibake.c

clean:
	rm -f example

.PHONY: all example clean
