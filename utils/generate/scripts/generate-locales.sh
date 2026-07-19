#!/bin/sh

# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

set -e

GENERATOR_DIR=$(CDPATH='' cd "$(dirname "$0")/.." && pwd)
cd "$GENERATOR_DIR"

if [ ! -f "./locales/ISO-639-2.txt" ] ; then
    curl -o locales/ISO-639-2.txt "https://www.loc.gov/standards/iso639-2/ISO-639-2_utf-8.txt"
fi

npm run generate -- "generate-locale"
