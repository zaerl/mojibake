#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

mkdir -p ../../build-amalgamation
mkdir -p ../../build-embedded-amalgamation

npm run generate -- embedded-db
npm run generate -- amalgamation
npm run generate -- embedded-amalgamation

# Copy database
cp ../../mojibake.db ../../build-amalgamation/mojibake.db

# Copy WASM files
cp ../../build-wasm/src/mojibake.js ../../build-amalgamation/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../build-amalgamation/mojibake.wasm

cp ../../build-wasm/src/mojibake.js ../../src/api/mojibake.js
cp ../../build-wasm/src/mojibake.wasm ../../src/api/mojibake.wasm
cp ../../mojibake.db ../../src/api/mojibake.db
cp ./functions.json ../../src/api/functions.json

VERSION=$(cat ../../VERSION | tr -d ' \n.' )

cd ../../build-amalgamation
rm -f ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip

echo "Creating amalgamation zip file..."
zip ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip mojibake.h mojibake.c mojibake.db

echo "Creating WASM zip file..."
zip ../build-wasm/src/mojibake-wasm-${VERSION}.zip mojibake.js mojibake.wasm mojibake.db

cd ../build-embedded-amalgamation
rm -f ../build-wasm/src/mojibake-embedded-amalgamation-${VERSION}.zip

echo "Creating embedded amalgamation zip file..."
zip ../build-wasm/src/mojibake-embedded-amalgamation-${VERSION}.zip mojibake.h mojibake.c
