#!/bin/bash

clang -o ../build/test main.c ../src/*.c && ../build/test
