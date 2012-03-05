//===-- RigelSelectionDAGInfo.cpp - Rigel SelectionDAG Info -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the RigelSelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "rigel-selectiondag-info"
#include "RigelTargetMachine.h"
using namespace llvm;

RigelSelectionDAGInfo::RigelSelectionDAGInfo(const RigelTargetMachine &TM)
  : TargetSelectionDAGInfo(TM) {
}

RigelSelectionDAGInfo::~RigelSelectionDAGInfo() {
}
