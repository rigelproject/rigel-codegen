//=====-- RigelTargetAsmInfo.h - Rigel asm properties -----------*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the RigelTargetAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef RIGELTARGETASMINFO_H
#define RIGELTARGETASMINFO_H

#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCAsmInfo.h"


namespace llvm {

  // Forward declaration.
  class Target;

  class RigelMCAsmInfo : public MCAsmInfo {
  public:

    explicit RigelMCAsmInfo(const Target &T, StringRef TT);

  };

} // namespace llvm

#endif
