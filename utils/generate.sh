#!/bin/sh

UNICODE_VERSION="13.0.0"
SQLITE_YEAR="2020"
SQLITE_VERSION="3330000"

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

if [ ! -d "./sqlite-$SQLITE_VERSION" ] ; then
    curl -o sqlite.zip "https://sqlite.com/$SQLITE_YEAR/sqlite-amalgamation-$SQLITE_VERSION.zip"
    unzip sqlite.zip
    mv "sqlite-amalgamation-$SQLITE_VERSION" "sqlite-$SQLITE_VERSION"
    rm sqlite.zip
    cp "sqlite-$SQLITE_VERSION/sqlite3.h" ../src/sqlite
    cp "sqlite-$SQLITE_VERSION/sqlite3.c" ../src/sqlite
    clang -o ../cli/sqlite3 ./sqlite/*.c && chmod +x ../cli/sqlite3
fi

npm run generate -- "$@"
