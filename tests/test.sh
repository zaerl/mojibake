#!/bin/bash

clang -Werror -o ../build/test main.c ../src/*.c ../src/sqlite/sqlite3.c && ../build/test
