#!/bin/bash

function create_header
{
    HEADER=$1

    FILENAME=$(basename $HEADER)
    DEFINE=$(echo $FILENAME | tr '[:lower:]' '[:upper:]' | tr -d '[:punct:]' )
    echo "Creating forward header $FILENAME"

    echo "" > $FILENAME
    echo "#ifndef PODOFO_WRAPPER_$DEFINE" >> $FILENAME
    echo "#define PODOFO_WRAPPER_$DEFINE" >> $FILENAME
    echo "/*" >> $FILENAME
    echo " * This is a simple wrapper include file that lets you include" >> $FILENAME
    echo " * <podofo/base/$FILENAME> when building against a podofo build directory" >> $FILENAME
    echo " * rather than an installed copy of podofo. You'll probably need" >> $FILENAME
    echo " * this if you're including your own (probably static) copy of podofo" >> $FILENAME
    echo " * using a mechanism like svn:externals ." >> $FILENAME
    echo " */" >> $FILENAME
    echo "#include \"$HEADER\"" >> $FILENAME
    echo "#endif" >> $FILENAME
}

HEADERS=$(find ../../src/base -name '*.h') 
for HEADER in $HEADERS;
do
    create_header $HEADER 
done

cd util
UTIL_HEADERS=$(find ../../../src/base/util -name '*.h') 
for HEADER in $UTIL_HEADERS;
do
    create_header $HEADER
done
cd ..
