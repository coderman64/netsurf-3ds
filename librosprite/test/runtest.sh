#!/bin/sh 

# run test images through librosprite and count results

TEST_PATH=$1
TEST_OUT=${TEST_PATH}/ppm
TEST_LOG=${TEST_PATH}/test.log

mkdir -p ${TEST_OUT}

echo "RO Sprite tests" > ${TEST_LOG}

# netsurf test sprites
SPRTESTS="test/sprite/*.spr"


rospritedecode()
{
    OUTF=$(basename ${1} .spr)
    CMPF=$(dirname  ${1})/${OUTF}.ppm
    echo "Icon:${1}" >> ${TEST_LOG}
    ${TEST_PATH}/test_decode_rosprite ${1} ${TEST_OUT}/${OUTF}.ppm 2>> ${TEST_LOG}
    if [ -f "${CMPF}" ]; then
	cmp ${CMPF} ${TEST_OUT}/${OUTF}.ppm >> ${TEST_LOG} 2>> ${TEST_LOG}
	if [ "$?" -ne 0 ]; then
	    return 128
	fi
    fi
}

# sprite tests

SPRTESTTOTC=0
SPRTESTPASSC=0
SPRTESTERRC=0

# netsurf test sprites
for SPR in $(ls ${SPRTESTS});do
    SPRTESTTOTC=$((SPRTESTTOTC+1))
    rospritedecode ${SPR}
    ECODE=$?
    if [ \( "${ECODE}" -gt 127 \) -o \( "${ECODE}" -eq 1 \) ];then
	SPRTESTERRC=$((SPRTESTERRC+1))
    else
	SPRTESTPASSC=$((SPRTESTPASSC+1))
    fi
done

echo "Test sprite decode"
echo "Tests:${SPRTESTTOTC} Pass:${SPRTESTPASSC} Error:${SPRTESTERRC}"


# exit code
if [ "${SPRTESTERRC}" -gt 0 ]; then
    exit 1
fi

exit 0
