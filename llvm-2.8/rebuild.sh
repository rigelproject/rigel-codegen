#!/usr/bin/env bash

# This script configures, builds, and installs LLVM 2.8 in accordance with
# the RIGEL_BUILD and RIGEL_INSTALL environment variables.

# NOTE: By default this will build a compiler and libraries for the current machine.
# If you want to generate a 32-bit compiler/libraries on a 64-bit machine, uncomment
# the following line:

#LLVMHOST=i686-pc-linux-gnu

if [ -z $LLVMHOST ]
then
  HOSTARG=--host=${LLVMHOST}
else
	HOSTARG=
fi

echo "[LLVM 2.8] Initializing";

: ${RIGEL_BUILD?"Need to set RIGEL_BUILD"}
: ${RIGEL_INSTALL?"Need to set RIGEL_INSTALL"}
: ${RIGEL_CODEGEN?"Need to set RIGEL_CODEGEN"}

THREADS=${RIGEL_MAKE_PAR:-4}
BUILD=${RIGEL_BUILD}/codegen/llvm-2.8
INSTALL=${RIGEL_INSTALL}/host
SRC=${RIGEL_CODEGEN}/llvm-2.8
THREADS=${RIGEL_MAKE_PAR:-4}
MAKE=${MAKE:-make}

pushd ${BUILD} >/dev/null

echo "[LLVM 2.8] Building source from '${SRC}' in '${BUILD}'"
$MAKE -j${THREADS}
if [ $? -ne 0 ]
then
	echo "[LLVM 2.8] BUILD FAILED"
	exit 1
fi

echo "[LLVM 2.8] Installing from '${BUILD}' to '${INSTALL}'"
$MAKE install
## check most recent foreground pipeline exit status for failure
if [ $? -ne 0 ]
then
	echo "[LLVM 2.8] INSTALL FAILED"
	exit 1
fi

echo "[LLVM 2.8] Success."

popd >/dev/null
