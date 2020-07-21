#!/bin/sh

last=false

usage() {
    echo "Usage: $0 [-l] name"
    exit 1
}

while getopts "lh" opt; do
    case $opt in
      l )
          last=true
          ;;
      h )
          usage
          ;;
    esac
done

shift $((OPTIND -1))

name=$(echo $@ | tr '[:lower:]' '[:upper:]')

if [ "$last" = false ] ; then

    cat <<EOB
/**
 * The UCX library
 *
 * This file is distributed under the MIT License. See LICENSE for details.
 */
#ifndef UCX_${name}_H
#define UCX_${name}_H

EOB

else

    cat <<EOE
#endif /* UCX_${name}_H */
EOE

fi
