#!/bin/sh

./load.sh

source="UCD/UnicodeData.txt"
lines=$(wc -l < $source)
lines=$(echo "${lines}" | sed -e 's/^[ ]*//')

# unicode_data.h
header=$(./header_guard.sh unicode_data)
footer=$(./header_guard.sh -l unicode_data)
license=$(./license.sh)
folder="../src"
data="${folder}/unicode_data"

cat > "${data}.h" <<- EOM
$header

#include "ucx.h"

#define UCX_CHARACTER_MAX ${lines}

extern ucx_character ucx_characters[UCX_CHARACTER_MAX];

$footer
EOM

# unicode_data.c
cat > "${data}.c" <<- EOM
${license}

#include "unicode_data.h"

ucx_character ucx_characters[] = {
EOM

awk -v "lines=$lines" -F ';' '{ printf "    { 0x" $1 ", 0, \"" $2 "\" }" } NR < lines { print "," }' $source >> "${data}.c"
echo "\n};" >> "${data}.c"

blocks="${folder}/blocks"

cat > "${data}.h" <<- EOM
$header

EOM



cat > "${data}.h" <<- EOM

$footer
EOM
