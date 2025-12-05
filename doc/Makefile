all: example example-embedded

# Compile example program
example: example.c ../build-amalgamation/mojibake.c
	$(CC) -o example example.c ../build-amalgamation/mojibake.c

example-embedded: example.c ../build-embedded-amalgamation/mojibake.c
	$(CC) -o example-embedded example.c ../build-embedded-amalgamation/mojibake.c

clean:
	rm -f example example-embedded

.PHONY: all example example-embedded clean
