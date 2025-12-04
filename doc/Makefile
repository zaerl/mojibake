all: example

# Compile example program
example: example.c ../build-amalgamation/mojibake.c
	$(CC) -o example example.c ../build-amalgamation/mojibake.c

clean:
	rm -f example

.PHONY: all example clean
