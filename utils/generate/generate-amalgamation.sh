#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

mkdir -p ../../build-amalgamation

npm run generate -- amalgamation

# Copy WASM files
cp ../../build-wasm/src/mojibake.js ../../build-amalgamation/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../build-amalgamation/mojibake.wasm

cp ../../build-wasm/src/mojibake.js ../../src/api/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../src/api/mojibake.wasm
cp ./functions.js ../../src/api/functions.js

VERSION=$(cat ../../VERSION | tr -d ' \n.' )

cd ../../build-amalgamation
rm -f ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip

echo "Creating amalgamation zip file..."
zip ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip mojibake.h mojibake.c

echo "Creating WASM zip file..."
zip ../build-wasm/src/mojibake-wasm-${VERSION}.zip mojibake.js mojibake.wasm
