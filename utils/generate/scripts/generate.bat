@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

setlocal

pushd "%~dp0.." || (
    echo Failed to enter the generator directory. 1>&2
    endlocal
    exit /b 1
)

REM TODO: change "draft" to 18.0.0 when the final version is released.
set UNICODE_VERSION=draft
set SECURITY_VERSION=18.0.0
set DATA_DIR=unicode-data

if not exist "%DATA_DIR%" mkdir "%DATA_DIR%" || goto :fail

REM Check if UCD directory exists
if not exist "%DATA_DIR%\UCD" (
    curl -o "%DATA_DIR%\UCD.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/ucd/UCD.zip" || goto :fail
    mkdir "%DATA_DIR%\UCD" || goto :fail
    tar -xf "%DATA_DIR%\UCD.zip" -C "%DATA_DIR%\UCD" || goto :fail
    del "%DATA_DIR%\UCD.zip" || goto :fail
)

REM Check if unihan directory exists
if not exist "%DATA_DIR%\unihan" (
    curl -o "%DATA_DIR%\unihan.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/ucd/Unihan.zip" || goto :fail
    mkdir "%DATA_DIR%\unihan" || goto :fail
    tar -xf "%DATA_DIR%\unihan.zip" -C "%DATA_DIR%\unihan" || goto :fail
    del "%DATA_DIR%\unihan.zip" || goto :fail
)

REM Check if emoji directory exists
if not exist "%DATA_DIR%\emoji" (
    mkdir "%DATA_DIR%\emoji" || goto :fail
    curl -o "%DATA_DIR%\emoji\ReadMe.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/ReadMe.txt" || goto :fail
    curl -o "%DATA_DIR%\emoji\emoji-sequences.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-sequences.txt" || goto :fail
    curl -o "%DATA_DIR%\emoji\emoji-test.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-test.txt" || goto :fail
    curl -o "%DATA_DIR%\emoji\emoji-zwj-sequences.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/emoji/emoji-zwj-sequences.txt" || goto :fail
)

REM Check if collation directory exists
if not exist "%DATA_DIR%\collation" (
    mkdir "%DATA_DIR%\collation" || goto :fail
    curl -o "%DATA_DIR%\collation\allkeys.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/uca/allkeys.txt" || goto :fail
    curl -o "%DATA_DIR%\collation\decomps.txt" "https://www.unicode.org/Public/%UNICODE_VERSION%/uca/decomps.txt" || goto :fail
    curl -o "%DATA_DIR%\CollationTest.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/uca/CollationTest.zip" || goto :fail
    tar -xf "%DATA_DIR%\CollationTest.zip" -C "%DATA_DIR%\collation" || goto :fail
    del "%DATA_DIR%\CollationTest.zip" || goto :fail
)

REM Check if security directory exists
if not exist "%DATA_DIR%\security" (
    mkdir "%DATA_DIR%\security" || goto :fail
    curl -o "%DATA_DIR%\uts39-data.zip" "https://www.unicode.org/Public/%UNICODE_VERSION%/security/uts39-data-%SECURITY_VERSION%.zip" || goto :fail
    tar -xf "%DATA_DIR%\uts39-data.zip" -C "%DATA_DIR%\security" || goto :fail
    del "%DATA_DIR%\uts39-data.zip" || goto :fail
)

node -e "const fs = require('fs'); const expected = '# Version: ' + process.argv[2]; const lines = fs.readFileSync(process.argv[1], 'utf8').split(/\r?\n/u); process.exit(lines.includes(expected) ? 0 : 1)" "%DATA_DIR%\security\confusables.txt" "%SECURITY_VERSION%"
if errorlevel 1 (
    echo Security data version mismatch in confusables.txt; expected %SECURITY_VERSION% 1>&2
    goto :fail
)

node -e "const fs = require('fs'); const expected = '# Version: ' + process.argv[2]; const lines = fs.readFileSync(process.argv[1], 'utf8').split(/\r?\n/u); process.exit(lines.includes(expected) ? 0 : 1)" "%DATA_DIR%\security\intentional.txt" "%SECURITY_VERSION%"
if errorlevel 1 (
    echo Security data version mismatch in intentional.txt; expected %SECURITY_VERSION% 1>&2
    goto :fail
)

REM Create build directory
if not exist "..\..\build" mkdir "..\..\build" || goto :fail

REM Run npm generate
call npm run generate -- %*
set "GENERATE_STATUS=%ERRORLEVEL%"

popd
endlocal & exit /b %GENERATE_STATUS%

:fail
set "GENERATE_STATUS=%ERRORLEVEL%"
if "%GENERATE_STATUS%"=="0" set "GENERATE_STATUS=1"
popd
endlocal & exit /b %GENERATE_STATUS%
