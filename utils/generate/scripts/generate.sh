#!/bin/sh

# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set -e

GENERATOR_DIR=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
cd "$GENERATOR_DIR"

# TODO: change "draft" to 18.0.0 when the final version is released.
UNICODE_VERSION="draft"
SECURITY_VERSION="18.0.0"
DATA_DIR="./unicode-data"

mkdir -p "$DATA_DIR"

if [ ! -d "$DATA_DIR/UCD" ] ; then
    curl -o "$DATA_DIR/UCD.zip" "https://www.unicode.org/Public/$UNICODE_VERSION/ucd/UCD.zip"
    unzip "$DATA_DIR/UCD.zip" -d "$DATA_DIR/UCD"
    rm "$DATA_DIR/UCD.zip"
fi

if [ ! -d "$DATA_DIR/unihan" ] ; then
    curl -o "$DATA_DIR/unihan.zip" "https://www.unicode.org/Public/$UNICODE_VERSION/ucd/Unihan.zip"
    unzip "$DATA_DIR/unihan.zip" -d "$DATA_DIR/unihan"
    rm "$DATA_DIR/unihan.zip"
fi

if [ ! -d "$DATA_DIR/emoji" ] ; then
    mkdir -p "$DATA_DIR/emoji"

    for file in "ReadMe.txt" "emoji-sequences.txt" "emoji-test.txt" "emoji-zwj-sequences.txt"; do
        curl -o "$DATA_DIR/emoji/$file" "https://www.unicode.org/Public/$UNICODE_VERSION/emoji/$file"
    done
fi

if [ ! -d "$DATA_DIR/collation" ] ; then
    mkdir -p "$DATA_DIR/collation"

    for file in "allkeys.txt" "decomps.txt"; do
        curl -o "$DATA_DIR/collation/$file" "https://www.unicode.org/Public/$UNICODE_VERSION/uca/$file"
    done

    curl -o "$DATA_DIR/CollationTest.zip" "https://www.unicode.org/Public/$UNICODE_VERSION/uca/CollationTest.zip"
    unzip "$DATA_DIR/CollationTest.zip" -d "$DATA_DIR/collation"
    rm "$DATA_DIR/CollationTest.zip"
fi

if [ ! -d "$DATA_DIR/security" ] ; then
    mkdir -p "$DATA_DIR/security"
    curl -o "$DATA_DIR/uts39-data.zip" \
        "https://www.unicode.org/Public/$UNICODE_VERSION/security/uts39-data-$SECURITY_VERSION.zip"
    unzip "$DATA_DIR/uts39-data.zip" -d "$DATA_DIR/security"
    rm "$DATA_DIR/uts39-data.zip"
fi

for file in "confusables.txt" "intentional.txt"; do
    if ! grep -q "^# Version: $SECURITY_VERSION$" "$DATA_DIR/security/$file"; then
        echo "Security data version mismatch in $file; expected $SECURITY_VERSION" >&2
        exit 1
    fi
done

mkdir -p ../../build
npm run generate -- "$@"
