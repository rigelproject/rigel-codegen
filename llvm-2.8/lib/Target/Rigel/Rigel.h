//===-- Rigel.h - Top-level interface for Rigel representation ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in 
// the LLVM Rigel back-end.
//
//===----------------------------------------------------------------------===//

#ifndef TARGET_RIGEL_H
#define TARGET_RIGEL_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {
  class RigelTargetMachine;
  class FunctionPass;
  class MachineCodeEmitter;
  class formatted_raw_ostream;

  FunctionPass *createRigelISelDag(RigelTargetMachine &TM);
//  FunctionPass *createRigelDelaySlotFillerPass(RigelTargetMachine &TM);
  //FunctionPass *createRigelCodePrinterPass(raw_ostream &OS, 
                                          //RigelTargetMachine &TM);
  extern Target TheRigelTarget;
  //extern Target TheRigelelTarget;
} // end namespace llvm;

// Defines symbolic names for Rigel registers.  This defines a mapping from
// register name to register number.
#include "RigelGenRegisterNames.inc"

// Defines symbolic names for the Rigel instructions.
#include "RigelGenInstrNames.inc"

#endif
