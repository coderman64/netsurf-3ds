#!/bin/sh

outline() {
echo >>${LOGFILE}
echo "-----------------------------------------------------------"  >>${LOGFILE}
echo >>${LOGFILE}
}

BUILDDIR=$1
TESTSRCDIR=$2

# locations
# test output
TESTOUTDIR=${BUILDDIR}/test
# test overall output
LOGFILE=${TESTOUTDIR}/testlog

# genbind tool
NSGENBIND=${BUILDDIR}/nsgenbind

#bindings
BINDINGDIR=${TESTSRCDIR}/data/bindings
BINDINGTESTS=$(ls ${BINDINGDIR}/*.bnd)

IDLDIR=${TESTSRCDIR}/data/idl

mkdir -p ${TESTOUTDIR}

echo "$*" >${LOGFILE}

for TEST in ${BINDINGTESTS};do

  outline

  TESTNAME=$(basename ${TEST} .bnd)
  TESTDIR=${TESTOUTDIR}/${TESTNAME}

  echo -n "    TEST: ${TESTNAME}......"
  echo "    TEST: ${TESTNAME}......" >>${LOGFILE}

  mkdir -p ${TESTDIR}
  # per test results
  RESFILE=${TESTDIR}/testres
  # per test errors
  ERRFILE=${TESTDIR}/testerr

  echo  ${NSGENBIND} -v -D -g -I ${IDLDIR} ${TEST} ${TESTOUTDIR}/${TESTNAME} >>${LOGFILE} 2>&1

  ${NSGENBIND} -v -D -g -I ${IDLDIR} ${TEST} ${TESTOUTDIR}/${TESTNAME} >${RESFILE} 2>${ERRFILE}

  RESULT=$?

  echo >> ${LOGFILE}
  cat ${ERRFILE} >> ${LOGFILE}
  echo >> ${LOGFILE}
  cat ${RESFILE} >> ${LOGFILE}

  if [ ${RESULT} -eq 0 ]; then
    echo "PASS"
  else
    echo "FAIL"
  fi
  

done

