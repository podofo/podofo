#!/bin/bash

if [ $# -ne 1 ];
    then
    echo "Usage:"
    echo "./podofo-system.sh PATH_TO_PARSER_TEST_EXECUTABLE"

    exit -1
fi


BAVARIA_DOWNLOAD="http://www.pdflib.com/fileadmin/pdflib/Bavaria/2009-04-03-Bavaria-pdfa.zip"
BAVARIA_NAME="2009-04-03-Bavaria-pdfa.zip"
DATA_DIR=data
OUTPUT_DIR=output/$(date +%Y%m%d-%H:%M)
PREFIX_OUT="podofo_"
SUFFIX_LOG=".log"
PARSER_TEST=$1
FAILED_TESTS=""

echo "Starting PoDoFo system test session."
echo ""
echo "Reading input data from: $DATA_DIR"
echo "Writing output to      : $OUTPUT_DIR"
echo "Using parser test      : $PARSER_TEST"
echo ""

if [ ! -e $DATA_DIR/$BAVARIA_NAME ];
    then
    echo "Bavaria test suite missing, downloading ..."
    wget -O $DATA_DIR/$BAVARIA_NAME $BAVARIA_DOWNLOAD
    unzip -d $DATA_DIR $DATA_DIR/$BAVARIA_NAME
    echo "Download done."
fi

mkdir -p $OUTPUT_DIR

count=0
errors=0

SAVED_IFS=$IFS # Make for loop work with filenames containing spaces
IFS=$(echo -en "\n\b")
FILES=$(find $DATA_DIR -name '*.pdf' -print)
for file in $FILES
  do
    FILENAME=$(basename "$file")

    echo -n -e "Running system test for: $file "

    $PARSER_TEST "$file" "$OUTPUT_DIR/$PREFIX_OUT$FILENAME" &> "$OUTPUT_DIR/$PREFIX_OUT$FILENAME$SUFFIX_LOG"
    RET=$?
    
    export count=$(( $count + 1 ))

    if [ $RET -eq 0 ];
	then
	echo -n -e "OK\n"
    else
	export errors=$(( $errors + 1))
	echo -n -e "FAIL\n"
	FAILED_TESTS="$FAILED_TESTS\n$file"
    fi
done

echo ""
echo "Number of files parsed  : "$count
echo "Number of files in error: "$errors
echo ""
IFS=$SAVED_IFS

if [ $errors -eq 0 ];
    then
    echo "SUCCESS"
    exit 0
else
    echo "List of failed tests:"
    echo -e $FAILED_TESTS
    echo ""
    echo "FAILED"
    exit -2
fi
