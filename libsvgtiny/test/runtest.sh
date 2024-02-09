#!/bin/sh 
#
# run test images through libnssvg and count results
# This file is part of libsvgtiny
#
# Licensed under the MIT License,
#                http://opensource.org/licenses/mit-license.php
# Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>

TEST_PATH=$1
TEST_OUT=${TEST_PATH}/mvg
TEST_LOG=${TEST_PATH}/test.log

mkdir -p ${TEST_OUT}

echo "SVG tests" > ${TEST_LOG}

# svg test directories

# netsurf test svg
SVGTESTS="test/data/*.svg"

# netsurf afl generated svg suite
SVGTESTS="${SVGTESTS} test/ns-afl-svg/*.svg"

svgdecode()
{
    OUTF=$(basename ${1} .svg)
    CMPF=$(dirname  ${1})/${OUTF}.mvg
    echo "SVG:${1}" >> ${TEST_LOG}
    ${TEST_PATH}/test_decode_svg ${1} 1.0 ${TEST_OUT}/${OUTF}.mvg 2>> ${TEST_LOG}
    ECODE=$?

    echo "Exit code:${ECODE}" >> ${TEST_LOG}
    if [ "${ECODE}" -gt 0 ];then
	return ${ECODE}
    fi

    if [ -f "${CMPF}" ]; then
	cmp ${CMPF} ${TEST_OUT}/${OUTF}.ppm >> ${TEST_LOG} 2>> ${TEST_LOG}
	if [ "$?" -ne 0 ]; then
	    return 128
	fi
    fi

    #convert mvg:${TEST_OUT}/${OUTF}.mvg png:${TEST_OUT}/${OUTF}.png

    return 0
}

SVGTESTTOTC=0
SVGTESTPASSC=0
SVGTESTFAILC=0
SVGTESTERRC=0

echo "Testing SVG decode"

for SVG in $(ls ${SVGTESTS});do
    SVGTESTTOTC=$((SVGTESTTOTC+1))
    svgdecode ${SVG}
    ECODE=$?
    if [ "${ECODE}" -gt 127 ];then
	SVGTESTERRC=$((SVGTESTERRC+1))
    else
	if [ "${ECODE}" -gt 0 ];then
	    SVGTESTFAILC=$((SVGTESTFAILC+1))
	else
	    SVGTESTPASSC=$((SVGTESTPASSC+1))
	fi
    fi
done

echo "Tests:${SVGTESTTOTC} Pass:${SVGTESTPASSC} Fail:${SVGTESTFAILC} Error:${SVGTESTERRC}"

# exit code
if [ "${SVGTESTFAILC}" -gt 0 ]; then
    exit 1
fi
if [ "${SVGTESTERRC}" -gt 0 ]; then
    exit 2
fi

exit 0
