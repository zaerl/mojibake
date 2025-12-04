#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

mkdir -p ../../build-amalgamation
bun run generate -- amalgamation

cp ../../mojibake.db ../../build-amalgamation/mojibake.db
cp ../../build-wasm/src/mojibake.js ../../build-amalgamation/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../build-amalgamation/mojibake.wasm

VERSION=$(cat ../../VERSION | tr -d ' \n.' )

cd ../../build-amalgamation
zip ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip mojibake.h mojibake.c mojibake.db
zip ../build-wasm/src/mojibake-wasm-${VERSION}.zip mojibake.js mojibake.wasm mojibake.db
