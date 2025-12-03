#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

mkdir -p ../../build-amalgamation
bun run generate -- amalgamation

cp ../../mojibake.db ../../build-amalgamation/mojibake.db

VERSION=$(cat ../../VERSION | tr -d ' \n.' )

cd ../../build-amalgamation
zip -r ../build-wasm/src/mojibake-amalgamation-${VERSION}.zip .
