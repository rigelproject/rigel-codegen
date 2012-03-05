//===-- RigelTargetAsmInfo.cpp - Rigel asm properties -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the RigelTargetAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "RigelMCAsmInfo.h"
using namespace llvm;

RigelMCAsmInfo::RigelMCAsmInfo(const Target &T, StringRef TT) {
  WeakRefDirective = "\t.weak\t";	
  AlignmentIsInBytes          = false;
  Data16bitsDirective = "\t.half\t";
  Data32bitsDirective = "\t.word\t";
  CommentString = "#";
}
