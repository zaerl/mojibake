#!/bin/sh

# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set -e

GENERATOR_DIR=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
cd "$GENERATOR_DIR"

mkdir -p ../../build-amalgamation

npm run generate -- amalgamation

# Copy WASM files
cp ../../build-wasm/src/mojibake.js ../../build-amalgamation/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../build-amalgamation/mojibake.wasm

cp ../../build-wasm/src/mojibake.js ../../src/api/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../src/api/mojibake.wasm

VERSION=$(tr -d ' \n.' < ../../VERSION)

cd ../../build-amalgamation
rm -f ../build-wasm/src/mojibake-amalgamation-"${VERSION}".zip

echo "Creating amalgamation zip file..."
zip ../build-wasm/src/mojibake-amalgamation-"${VERSION}".zip mojibake.h mojibake.c shell.c

echo "Creating WASM zip file..."
zip ../build-wasm/src/mojibake-wasm-"${VERSION}".zip mojibake.js mojibake.wasm
