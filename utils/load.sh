#!/bin/sh

UNICODE_VERSION="13.0.0"
SQLITE_YEAR="2020"
SQLITE_VERSION="3320300"

if [ ! -d "./UCD" ] ; then
    curl -o UCD.zip "https://www.unicode.org/Public/zipped/$UNICODE_VERSION/UCD.zip"
    unzip UCD.zip -d "UCD"
    rm UCD.zip
else
    echo "Skip UCD"
fi

if [ ! -d "./unihan" ] ; then
    curl -o unihan.zip "https://www.unicode.org/Public/zipped/$UNICODE_VERSION/Unihan.zip"
    unzip unihan.zip -d "unihan"
    rm unihan.zip
else
    echo "Skip unihan"
fi\

if [ ! -d "./sqlite" ] ; then
    curl -o sqlite.zip "https://sqlite.com/$SQLITE_YEAR/sqlite-amalgamation-$SQLITE_VERSION.zip"
    unzip sqlite.zip
    mv "sqlite-amalgamation-$SQLITE_VERSION" sqlite
    rm sqlite.zip
    mkdir ../src/sqlite
    mv sqlite/sqlite3.h ../src/sqlite/sqlite3.h
    mv sqlite/sqlite3.c ../src/sqlite/sqlite3.c
else
    echo "Skip sqlite"
fi
