@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

setlocal
pushd "%~dp0.." || exit /b 1

if not exist "locales\ISO-639-2.txt" (
    if not exist "locales" mkdir locales
    curl -o locales\ISO-639-2.txt "https://www.loc.gov/standards/iso639-2/ISO-639-2_utf-8.txt"
)

REM Run npm generate for locales
call npm run generate -- "generate-locale"
set "GENERATE_LOCALES_STATUS=%ERRORLEVEL%"

popd
endlocal & exit /b %GENERATE_LOCALES_STATUS%
