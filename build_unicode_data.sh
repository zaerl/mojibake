#!/bin/sh

lines=$(wc -l < $1)
lines=$(echo "${lines}" | sed -e 's/^[ ]*//')

# unicode_data.h
cat > unicode_data.h <<- EOM
/*
 * The Unidex library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
*/
#ifndef UNICODE_DATA_H
#define UNICODE_DATA_H

#include "unicodex.h"

#define UCX_CHARACTER_MAX ${lines}

extern ucx_character ucx_characters[UCX_CHARACTER_MAX];

#endif /* UNICODE_DATA_H */
EOM

# unicode_data.c
cat > unicode_data.c <<- EOM
/*
 The Unidex library
 */
#include "unicode_data.h"

ucx_character ucx_characters[] = {
EOM

awk -v "lines=$lines" -F ';' '{ printf "    { 0x" $1 ", 0, \"" $2 "\" }" } NR < lines { print "," }' $1 >> unicode_data.c
echo "\n};" >> unicode_data.c
