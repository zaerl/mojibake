#!/bin/bash

clang -std=c99 -Werror -o ../build/test *.c ../src/*.c && ../build/test "$@"
