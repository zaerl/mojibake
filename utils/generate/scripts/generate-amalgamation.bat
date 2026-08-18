@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

setlocal enabledelayedexpansion

pushd "%~dp0.." || exit /b 1

if not exist "..\..\build-amalgamation" mkdir "..\..\build-amalgamation"

call npm run generate -- amalgamation
if errorlevel 1 goto :native_amalgamation_error

REM Copy native amalgamation files to the macOS app
copy /Y "..\..\build-amalgamation\mojibake.c" "..\..\src\ui\Mojibake\Mojibake\mojibake.c" >NUL
if errorlevel 1 goto :native_amalgamation_error
copy /Y "..\..\build-amalgamation\mojibake.h" "..\..\src\ui\Mojibake\Mojibake\mojibake.h" >NUL
if errorlevel 1 goto :native_amalgamation_error

REM Copy WASM files
copy /Y "..\..\build-wasm\src\mojibake.js" "..\..\build-amalgamation\mojibake.js" >NUL
copy /Y "..\..\build-wasm\src\mojibake.wasm" "..\..\build-amalgamation\mojibake.wasm" >NUL

copy /Y "..\..\build-wasm\src\mojibake.js" "..\..\src\api\mojibake.js" >NUL
copy /Y "..\..\build-wasm\src\mojibake.wasm" "..\..\src\api\mojibake.wasm" >NUL

set /P VERSION=<"..\..\VERSION"
set "VERSION=!VERSION: =!"
set "VERSION=!VERSION:.=!"

pushd "..\..\build-amalgamation"
if exist "..\build-wasm\src\mojibake-amalgamation-!VERSION!.zip" (
    del "..\build-wasm\src\mojibake-amalgamation-!VERSION!.zip"
)

echo Creating amalgamation zip file...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -LiteralPath 'mojibake.h','mojibake.c','shell.c' -DestinationPath '..\build-wasm\src\mojibake-amalgamation-!VERSION!.zip' -Force"

echo Creating WASM zip file...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -LiteralPath 'mojibake.js','mojibake.wasm' -DestinationPath '..\build-wasm\src\mojibake-wasm-!VERSION!.zip' -Force"
set "GENERATE_AMALGAMATION_STATUS=%ERRORLEVEL%"

popd
popd
endlocal & exit /b %GENERATE_AMALGAMATION_STATUS%

:native_amalgamation_error
popd
endlocal & exit /b 1
