@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

if not exist "locales\ISO-639-2.txt" (
    if not exist "locales" mkdir locales
    curl -o locales\ISO-639-2.txt "https://www.loc.gov/standards/iso639-2/ISO-639-2_utf-8.txt"
)

REM Run npm generate for locales
npm run generate -- "locales"
