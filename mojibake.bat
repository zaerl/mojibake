@echo off

REM The Mojibake library
REM This file is distributed under the MIT License. See LICENSE for details.

set WRD_DB_PATH=%CD%\mojibake.db
build\src\shell\mojibake.exe %*
