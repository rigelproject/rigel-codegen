#!/usr/bin/env bash

# This script configures, builds, and installs binutils in accordance with
# the RIGEL_BUILD and RIGEL_INSTALL environment variables.

# NOTE: By default this will build binaries and libraries runnable on the current machine.
# If you want to generate 32-bit binaries/libraries on a 64-bit machine, uncomment
# the following line:

#BINUTILSHOST=i686-pc-linux-gnu

if [ -z $BINUTILSHOST ]
then
  HOSTARG=--host=${BINUTILSHOST}
else
	HOSTARG=
fi

echo "[BINUTILS] Initializing";

: ${RIGEL_BUILD?"Need to set RIGEL_BUILD"}
: ${RIGEL_INSTALL?"Need to set RIGEL_INSTALL"}
: ${RIGEL_CODEGEN?"Need to set RIGEL_CODEGEN"}

THREADS=${RIGEL_MAKE_PAR:-4}
BUILD=${RIGEL_BUILD}/codegen/binutils-2.18
INSTALL=${RIGEL_INSTALL}/host
SRC=${RIGEL_CODEGEN}/binutils-2.18
MAKE=${MAKE:-make}

if [ -d "${BUILD}" ]
then
	echo "[BINUTILS] BUILD DIRECTORY '${BUILD}' EXISTS."
	read -p "ERASE CONTENTS AND REBUILD? (y/n): " -n 1 -r
	if [[ $REPLY =~ ^[Yy]$ ]]
	then
		echo
	  rm -rf ${BUILD}/*
	else
		echo
		echo "[BINUTILS] Exiting."
	fi
else
  echo "[BINUTILS] Creating build directory '${BUILD}'"
  mkdir -p ${BUILD}
fi

pushd ${BUILD} >/dev/null

echo "[BINUTILS] Configuring in '${BUILD}'"
${SRC}/configure ${HOSTARG} --enable-static --enable-shared --program-prefix=rigel --target=mips --prefix=${INSTALL} --disable-option-checking ${EXTRA_BINUTILS_CONFIGURE_ARGS}
if [ $? -ne 0 ]
then
	echo "[BINUTILS] CONFIGURE FAILED"
	exit 1
fi

echo "[BINUTILS] Building source from '${SRC}' in '${BUILD}'"
$MAKE -j${THREADS}
if [ $? -ne 0 ]
then
	echo "[BINUTILS] BUILD FAILED"
	exit 1
fi

echo "[BINUTILS] Installing from '${BUILD}' to '${INSTALL}'"
$MAKE install
if [ $? -ne 0 ]
then
	echo "[BINUTILS] INSTALL FAILED"
	exit 1
fi

echo "[BINUTILS] Success."

popd >/dev/null
