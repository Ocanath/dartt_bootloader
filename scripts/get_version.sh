#!/bin/bash

X=$(git rev-parse --short HEAD)
Y=$(git tag --points-at HEAD)

echo -------------
echo git rev \(short\):
echo "$X"
echo -------------

FPATH="../shared/version.h"
echo "$FPATH"

{
    echo "#ifndef VERSION_H"
    echo "#define VERSION_H"
    echo ""
    echo "static const char firmware_version[] = \"$X\";"
    echo "static const char firmware_tag[] = \"$Y\";"
    echo ""
    echo "#endif"
    echo ""
} > "$FPATH"
