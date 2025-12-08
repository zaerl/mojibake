@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

setlocal enabledelayedexpansion

set UNICODE_VERSION=17.0.0

REM Check if UCD directory exists
if not exist "UCD" (
    curl -o UCD.zip "https://www.unicode.org/Public/%UNICODE_VERSION%/ucd/UCD.zip"
    mkdir UCD
    tar -xf UCD.zip -C UCD
    del UCD.zip
)

REM Check if unihan directory exists
if not exist "unihan" (
    curl -o unihan.zip "https://www.unicode.org/Public/%UNICODE_VERSION%/ucd/Unihan.zip"
    mkdir unihan
    tar -xf unihan.zip -C unihan
    del unihan.zip
)

REM Check if emoji directory exists
if not exist "emoji" (
    mkdir emoji
    curl -o "emoji\ReadMe.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/ReadMe.txt"
    curl -o "emoji\emoji-sequences.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-sequences.txt"
    curl -o "emoji\emoji-test.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-test.txt"
    curl -o "emoji\emoji-zwj-sequences.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-zwj-sequences.txt"
)

REM Create build directory and remove old database
if not exist "..\..\build" mkdir "..\..\build"
if exist "..\..\mojibake.db" del "..\..\mojibake.db"

REM Run npm generate
npm run generate -- %*

endlocal
