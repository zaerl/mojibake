#!/bin/bash

clang -o build/test tests/main.c src/*.c && ./build/test
