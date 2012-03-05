//===-- RigelSelectionDAGInfo.h - Rigel SelectionDAG Info ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Rigel subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef RIGELSELECTIONDAGINFO_H
#define RIGELSELECTIONDAGINFO_H

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class RigelTargetMachine;

class RigelSelectionDAGInfo : public TargetSelectionDAGInfo {
public:
  explicit RigelSelectionDAGInfo(const RigelTargetMachine &TM);
  ~RigelSelectionDAGInfo();
};

} // namespace llvm

#endif
