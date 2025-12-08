#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

UNICODE_VERSION="17.0.0"

if [ ! -d "./UCD" ] ; then
    curl -o UCD.zip "https://www.unicode.org/Public/$UNICODE_VERSION/ucd/UCD.zip"
    unzip UCD.zip -d "UCD"
    rm UCD.zip
fi

if [ ! -d "./unihan" ] ; then
    curl -o unihan.zip "https://www.unicode.org/Public/$UNICODE_VERSION/ucd/Unihan.zip"
    unzip unihan.zip -d "unihan"
    rm unihan.zip
fi

if [ ! -d "./emoji" ] ; then
    mkdir -p "./emoji"

    for file in "ReadMe.txt" "emoji-sequences.txt" "emoji-test.txt" "emoji-zwj-sequences.txt"; do
        curl -o "./emoji/$file" "https://www.unicode.org/Public/$UNICODE_VERSION/emoji/$file"
    done
fi


mkdir -p ../../build
rm -f ../../mojibake.db
bun run generate -- "$@"
