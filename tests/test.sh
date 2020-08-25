#!/bin/bash

clang -std=c99 -Werror -o ../build/test *.c ../src/*.c ../src/sqlite/sqlite3.c && ../build/test "$@"
