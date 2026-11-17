@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

REM Download only the Unicode data files read by the test suite. Unlike generate.bat, this does
REM not fetch the full UCD or run the table generator. Files already present are kept. This is the
REM Windows counterpart of download-test-data.sh; keep the two file lists in sync.

setlocal

pushd "%~dp0..\..\.." || (
    echo Failed to enter the repository root. 1>&2
    endlocal
    exit /b 1
)

set UNICODE_VERSION=18.0.0
set SECURITY_VERSION=18.0.0
set IDNA_VERSION=18.0.0
set BASE_URL=https://www.unicode.org/Public/%UNICODE_VERSION%
set DATA_DIR=%CD%\unicode-data

REM Blocks.txt and the property files after it are read by the Elixir character-data tests.
for %%F in (BidiCharacterTest.txt BidiTest.txt CaseFolding.txt DerivedNormalizationProps.txt ^
    NameAliases.txt NormalizationTest.txt PropertyValueAliases.txt ScriptExtensions.txt ^
    SpecialCasing.txt UnicodeData.txt Blocks.txt DerivedCoreProperties.txt EastAsianWidth.txt ^
    LineBreak.txt PropList.txt Scripts.txt) do (
    call :fetch "%BASE_URL%/ucd/%%F" "%DATA_DIR%\UCD\%%F" || goto :fail
)
call :fetch "%BASE_URL%/ucd/emoji/emoji-data.txt" ^
    "%DATA_DIR%\UCD\emoji\emoji-data.txt" || goto :fail
call :fetch "%BASE_URL%/ucd/extracted/DerivedJoiningType.txt" ^
    "%DATA_DIR%\UCD\extracted\DerivedJoiningType.txt" || goto :fail

for %%F in (GraphemeBreakTest.txt LineBreakTest.txt SentenceBreakTest.txt WordBreakTest.txt ^
    GraphemeBreakProperty.txt SentenceBreakProperty.txt WordBreakProperty.txt) do (
    call :fetch "%BASE_URL%/ucd/auxiliary/%%F" "%DATA_DIR%\UCD\auxiliary\%%F" || goto :fail
)

call :fetch "%BASE_URL%/emoji/emoji-test.txt" "%DATA_DIR%\emoji\emoji-test.txt" || goto :fail
call :fetch "%BASE_URL%/idna/IdnaTestV2.txt" "%DATA_DIR%\idna\IdnaTestV2.txt" || goto :fail
call :check_version "%DATA_DIR%\idna\IdnaTestV2.txt" %IDNA_VERSION% IDNA || goto :fail

set SECURITY_MISSING=
if not exist "%DATA_DIR%\security\intentional.txt" set SECURITY_MISSING=1
if not exist "%DATA_DIR%\security\confusables.txt" set SECURITY_MISSING=1
if defined SECURITY_MISSING (
    if not exist "%DATA_DIR%\security" mkdir "%DATA_DIR%\security" || goto :fail
    call :fetch "%BASE_URL%/security/uts39-data-%SECURITY_VERSION%.zip" ^
        "%DATA_DIR%\uts39-data.zip" || goto :fail
    tar -xf "%DATA_DIR%\uts39-data.zip" -C "%DATA_DIR%\security" || goto :fail
    del "%DATA_DIR%\uts39-data.zip" || goto :fail
)

for %%F in (intentional.txt confusables.txt) do (
    call :check_version "%DATA_DIR%\security\%%F" %SECURITY_VERSION% Security || goto :fail
)

REM The collation test files are only distributed inside CollationTest.zip.
set COLLATION_MISSING=
if not exist "%DATA_DIR%\collation\CollationTest\CollationTest_NON_IGNORABLE.txt" ^
    set COLLATION_MISSING=1
if not exist "%DATA_DIR%\collation\CollationTest\CollationTest_SHIFTED.txt" ^
    set COLLATION_MISSING=1
if defined COLLATION_MISSING (
    if not exist "%DATA_DIR%\collation" mkdir "%DATA_DIR%\collation" || goto :fail
    call :fetch "%BASE_URL%/uca/CollationTest.zip" "%DATA_DIR%\CollationTest.zip" || goto :fail
    tar -xf "%DATA_DIR%\CollationTest.zip" -C "%DATA_DIR%\collation" ^
        "CollationTest/CollationTest_NON_IGNORABLE.txt" ^
        "CollationTest/CollationTest_SHIFTED.txt" || goto :fail
    del "%DATA_DIR%\CollationTest.zip" || goto :fail
)

echo Test data ready in %DATA_DIR%

popd
endlocal & exit /b 0

:fail
set "DOWNLOAD_STATUS=%ERRORLEVEL%"
if "%DOWNLOAD_STATUS%"=="0" set "DOWNLOAD_STATUS=1"
popd
endlocal & exit /b %DOWNLOAD_STATUS%

REM Download URL %1 to file %2 unless the file already exists.
:fetch
if exist "%~2" exit /b 0
if not exist "%~dp2" mkdir "%~dp2" || exit /b 1
echo Downloading %~1
curl -fsSL -o "%~2" "%~1" || exit /b 1
exit /b 0

REM Fail unless file %1 has a "# Version: %2" header line; %3 names the data set.
:check_version
findstr /B /C:"# Version: %~2" "%~1" >NUL || (
    echo %~3 data version mismatch in %~nx1; expected %~2 1>&2
    exit /b 1
)
exit /b 0
