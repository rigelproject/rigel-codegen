//===- RigelSubtarget.cpp - Rigel Subtarget Information -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Rigel specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "RigelSubtarget.h"
#include "Rigel.h"
#include "RigelGenSubtarget.inc"
using namespace llvm;

RigelSubtarget::RigelSubtarget(const std::string &TT, const std::string &FS) :
	HasByteLdSt(false),
  StackAlignment(4)
{
  std::string CPU = "rstructural";

  // Parse features string.
  ParseSubtargetFeatures(FS, CPU);
}
