Codegen
=======

The codegen repository contains everything needed to cross-compile C and
assembly code for Rigel on a host machine. 

We use clang 2.8 as a C89, C99, and assembly front-end, LLVM 2.8 for
middle-end optimizations, an LLVM backend to generate assembly code, and GNU
binutils 2.18 to assemble and link binaries.

Organization
------------

### ./binutils-2.18

A version of GNU binutils with support for the Rigel instruction set.

#### Building

./binutils-2.18/build.sh builds and installs binutils

### ./llvm-2.8

A version of the LLVM compiler with a backend for the Rigel instruction set.

./llvm-2.8/build.sh builds and installs LLVM 2.8

./llvm-2.8/rebuild.sh is useful when a previous build and install has already
been performed.
