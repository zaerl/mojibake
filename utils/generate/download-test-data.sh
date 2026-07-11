#!/bin/sh

# The Mojibake library
#
# This file is distributed under the MIT License. See LICENSE for details.

# Download only the Unicode data files read by the test suite. Unlike generate.sh, this does
# not fetch the full UCD or run the table generator, so it is suitable for CI.
# See the main Dockerfile and the .github/workflows/test.yml for usage.

set -e

UNICODE_VERSION="17.0.0"
DATA_DIR="$(dirname "$0")/unicode-data"

fetch() {
    if [ ! -f "$2" ]; then
        mkdir -p "$(dirname "$2")"
        echo "Downloading $1"
        curl -fsSL -o "$2" "$1"
    fi
}

for file in "BidiCharacterTest.txt" "BidiTest.txt" "CaseFolding.txt" \
    "DerivedNormalizationProps.txt" "NormalizationTest.txt" "PropertyValueAliases.txt" \
    "ScriptExtensions.txt" "SpecialCasing.txt"; do
    fetch "https://www.unicode.org/Public/$UNICODE_VERSION/ucd/$file" "$DATA_DIR/UCD/$file"
done

for file in "GraphemeBreakTest.txt" "LineBreakTest.txt" "SentenceBreakTest.txt" \
    "WordBreakTest.txt"; do
    fetch "https://www.unicode.org/Public/$UNICODE_VERSION/ucd/auxiliary/$file" \
        "$DATA_DIR/UCD/auxiliary/$file"
done

fetch "https://www.unicode.org/Public/$UNICODE_VERSION/emoji/emoji-test.txt" \
    "$DATA_DIR/emoji/emoji-test.txt"
fetch "https://www.unicode.org/Public/security/latest/intentional.txt" \
    "$DATA_DIR/security/intentional.txt"
fetch "https://www.unicode.org/Public/security/latest/confusables.txt" \
    "$DATA_DIR/security/confusables.txt"

for file in "intentional.txt" "confusables.txt"; do
    if ! grep -q "^# Version: $UNICODE_VERSION$" "$DATA_DIR/security/$file"; then
        echo "Security data version mismatch in $file; expected $UNICODE_VERSION" >&2
        exit 1
    fi
done

# The collation test files are only distributed inside CollationTest.zip.
if [ ! -f "$DATA_DIR/collation/CollationTest/CollationTest_NON_IGNORABLE.txt" ] || \
    [ ! -f "$DATA_DIR/collation/CollationTest/CollationTest_SHIFTED.txt" ]; then
    mkdir -p "$DATA_DIR/collation"
    fetch "https://www.unicode.org/Public/$UNICODE_VERSION/uca/CollationTest.zip" \
        "$DATA_DIR/CollationTest.zip"

    if command -v unzip >/dev/null 2>&1; then
        unzip -o -q "$DATA_DIR/CollationTest.zip" \
            "CollationTest/CollationTest_NON_IGNORABLE.txt" \
            "CollationTest/CollationTest_SHIFTED.txt" -d "$DATA_DIR/collation"
    else
        # Windows runners have bsdtar, which extracts zip archives.
        tar -xf "$DATA_DIR/CollationTest.zip" -C "$DATA_DIR/collation" \
            "CollationTest/CollationTest_NON_IGNORABLE.txt" \
            "CollationTest/CollationTest_SHIFTED.txt"
    fi

    rm "$DATA_DIR/CollationTest.zip"
fi

echo "Test data ready in $DATA_DIR"
