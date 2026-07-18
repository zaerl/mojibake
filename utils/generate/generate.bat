@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

setlocal enabledelayedexpansion

REM TODO: change "draft" to 18.0.0 when the draft version is released.
set UNICODE_VERSION=draft
set SECURITY_VERSION=18.0.0
set DATA_DIR=unicode-data

if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"

REM Check if UCD directory exists
if not exist "%DATA_DIR%\UCD" (
    curl -o "%DATA_DIR%\UCD.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/ucd/UCD.zip"
    mkdir "%DATA_DIR%\UCD"
    tar -xf "%DATA_DIR%\UCD.zip" -C "%DATA_DIR%\UCD"
    del "%DATA_DIR%\UCD.zip"
)

REM Check if unihan directory exists
if not exist "%DATA_DIR%\unihan" (
    curl -o "%DATA_DIR%\unihan.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/ucd/Unihan.zip"
    mkdir "%DATA_DIR%\unihan"
    tar -xf "%DATA_DIR%\unihan.zip" -C "%DATA_DIR%\unihan"
    del "%DATA_DIR%\unihan.zip"
)

REM Check if emoji directory exists
if not exist "%DATA_DIR%\emoji" (
    mkdir "%DATA_DIR%\emoji"
    curl -o "%DATA_DIR%\emoji\ReadMe.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/ReadMe.txt"
    curl -o "%DATA_DIR%\emoji\emoji-sequences.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-sequences.txt"
    curl -o "%DATA_DIR%\emoji\emoji-test.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-test.txt"
    curl -o "%DATA_DIR%\emoji\emoji-zwj-sequences.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-zwj-sequences.txt"
)

REM Check if collation directory exists
if not exist "%DATA_DIR%\collation" (
    mkdir "%DATA_DIR%\collation"
    curl -o "%DATA_DIR%\collation\allkeys.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/uca/allkeys.txt"
    curl -o "%DATA_DIR%\collation\decomps.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/uca/decomps.txt"
    curl -o "%DATA_DIR%\CollationTest.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/uca/CollationTest.zip"
    tar -xf "%DATA_DIR%\CollationTest.zip" -C "%DATA_DIR%\collation"
    del "%DATA_DIR%\CollationTest.zip"
)

REM Check if security directory exists
if not exist "%DATA_DIR%\security" (
    mkdir "%DATA_DIR%\security"
    curl -o "%DATA_DIR%\uts39-data.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/security/uts39-data-%SECURITY_VERSION%.zip"
    tar -xf "%DATA_DIR%\uts39-data.zip" -C "%DATA_DIR%\security"
    del "%DATA_DIR%\uts39-data.zip"
)

findstr /x /c:"# Version: %SECURITY_VERSION%" "%DATA_DIR%\security\confusables.txt" >nul || exit /b 1
findstr /x /c:"# Version: %SECURITY_VERSION%" "%DATA_DIR%\security\intentional.txt" >nul || exit /b 1

REM Create build directory
if not exist "..\..\build" mkdir "..\..\build"

REM Run npm generate
call npm run generate -- %*
set "GENERATE_STATUS=%ERRORLEVEL%"

endlocal & exit /b %GENERATE_STATUS%
