#!/bin/bash

clang -Werror -o ../build/test main.c ../src/*.c && ../build/test
