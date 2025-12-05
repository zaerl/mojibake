#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

mkdir -p ../../build-amalgamation
mkdir -p ../../build-embedded-amalgamation

bun run generate -- embedded-db
bun run generate -- amalgamation
bun run generate -- embedded-amalgamation

# Copy database
cp ../../mojibake.db ../../build-amalgamation/mojibake.db

# Copy WASM files
cp ../../build-wasm/src/mojibake.js ../../build-amalgamation/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../build-amalgamation/mojibake.wasm

VERSION=$(cat ../../VERSION | tr -d ' \n.' )

cd ../../build-amalgamation
rm -f ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip
zip ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip mojibake.h mojibake.c mojibake.db
zip ../build-wasm/src/mojibake-wasm-${VERSION}.zip mojibake.js mojibake.wasm mojibake.db

cd ../build-embedded-amalgamation
rm -f ../build-wasm/src/mojibake-embedded-amalgamation-${VERSION}.zip
zip ../build-wasm/src/mojibake-embedded-amalgamation-${VERSION}.zip mojibake.h mojibake.c
