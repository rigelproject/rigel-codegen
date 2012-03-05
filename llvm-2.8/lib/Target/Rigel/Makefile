##===- lib/Target/Rigel/Makefile ----------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file was developed by the Rigel Team and is distributed under the 
# University of Illinois Open Source License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##
LEVEL = ../../..
LIBRARYNAME = LLVMRigelCodeGen
TARGET = Rigel

# Make sure that tblgen is run, first thing.
BUILT_SOURCES = RigelGenRegisterInfo.h.inc RigelGenRegisterNames.inc \
		RigelGenRegisterInfo.inc RigelGenInstrNames.inc \
		RigelGenInstrInfo.inc RigelGenAsmWriter.inc \
		RigelGenDAGISel.inc RigelGenCallingConv.inc \
		RigelGenSubtarget.inc

DIRS = AsmPrinter TargetInfo

include $(LEVEL)/Makefile.common

