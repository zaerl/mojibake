#!/bin/sh

UNICODE_VERSION="16.0.0"

if [ ! -d "./UCD" ] ; then
    curl -o UCD.zip "https://www.unicode.org/Public/zipped/$UNICODE_VERSION/UCD.zip"
    unzip UCD.zip -d "UCD"
    rm UCD.zip
fi

if [ ! -d "./unihan" ] ; then
    curl -o unihan.zip "https://www.unicode.org/Public/zipped/$UNICODE_VERSION/Unihan.zip"
    unzip unihan.zip -d "unihan"
    rm unihan.zip
fi

mkdir -p ../../build
rm ../../mojibake.db
npm run generate -- "$@"
