#!/bin/sh 

set -e

TEST_PATH=$1
TEST_PFX=$4

for TEST in basic explicitfilter; do
    ${TEST_PATH}/${TEST_PFX}${TEST}
done

exit 0
