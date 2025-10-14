#!/bin/sh

# The Mojibake library
# This file is distributed under the MIT License. See LICENSE for details.

WRD_DB_PATH="$(pwd)/mojibake.db" exec build/src/shell/mojibake "$@"
