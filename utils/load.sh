#!/bin/sh

if [ ! -d "./UCD" ] ; then
    curl -o UCD.zip "https://www.unicode.org/Public/zipped/13.0.0/UCD.zip"
    unzip UCD.zip -d "UCD"
    rm UCD.zip
    curl -o Unihan.zip "https://www.unicode.org/Public/zipped/13.0.0/Unihan.zip"
    unzip Unihan.zip -d "Unihan"
    rm Unihan.zip
fi

if [ ! -d "./Unihan" ] ; then
    curl -o Unihan.zip "https://www.unicode.org/Public/zipped/13.0.0/Unihan.zip"
    unzip Unihan.zip -d "Unihan"
    rm Unihan.zip
fi
