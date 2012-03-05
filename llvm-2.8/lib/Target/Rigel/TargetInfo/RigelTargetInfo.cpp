//===-- RigelTargetInfo.cpp - Rigel Target Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Rigel.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetRegistry.h"
using namespace llvm;

Target llvm::TheRigelTarget;

extern "C" void LLVMInitializeRigelTargetInfo() {
  RegisterTarget<Triple::rigel, /*HasJIT=*/false>
    X(TheRigelTarget, "rigel", "Rigel");
}
