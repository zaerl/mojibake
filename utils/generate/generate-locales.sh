#!/bin/sh

if [ ! -f "./locales/ISO-639-2.txt" ] ; then
    curl -o locales/ISO-639-2.txt "https://www.loc.gov/standards/iso639-2/ISO-639-2_utf-8.txt"
fi

npm run generate-locales
