#!/bin/sh

if [ ! -d "./UCD" ] ; then

    curl -o UCD.zip "https://www.unicode.org/Public/zipped/13.0.0/UCD.zip"
    unzip UCD.zip -d "UCD"
    rm UCD.zip

fi
