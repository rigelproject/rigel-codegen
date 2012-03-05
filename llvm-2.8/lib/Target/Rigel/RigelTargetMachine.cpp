//===-- RigelTargetMachine.cpp - Define TargetMachine for Rigel -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about Rigel target spec.
//
//===----------------------------------------------------------------------===//

#include "Rigel.h"
#include "RigelMCAsmInfo.h"
#include "RigelTargetMachine.h"
#include "llvm/PassManager.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/Target/TargetFrameInfo.h"

using namespace llvm;

extern "C" void LLVMInitializeRigelTarget() {
  // Register the target.
  RegisterTargetMachine<RigelTargetMachine> X(TheRigelTarget);
  RegisterAsmInfo<RigelMCAsmInfo>           A(TheRigelTarget);
}

// DataLayout --> Little-endian, 32-bit pointer/ABI/alignment
// The stack is always 4 byte aligned
// On function prologue, the stack is created by decrementing
// its pointer. Once decremented, all references are done with positive
// offset from the stack/frame pointer, so StackGrowsUp is used.
RigelTargetMachine::
RigelTargetMachine(const Target &T, const std::string &TT, const std::string &FS) :
  LLVMTargetMachine(T, TT),
  Subtarget(TT, FS),
  DataLayout(std::string("e-p:32:32:32-i1:8:8-i8:8:32-i16:16:32-i32:32:32-i64:32:64-f32:32:32-f64:32:64-a0:0:32-s0:32:32-n32")), // Rigel is LE
  InstrInfo(*this), 
  FrameInfo(TargetFrameInfo::StackGrowsUp, 4, 0, 4),
  TLInfo(*this), TSInfo(*this),
  InstrItins(Subtarget.getInstrItineraryData()) {
  //We need to set a relocation model explicitly if the user didn't specify one.
  //Otherwise, the object code emission stuff won't know what to do.
  if (getRelocationModel() == Reloc::Default) {
    setRelocationModel(Reloc::Static);
  }
}

// Install an instruction selector pass using 
// the ISelDag to gen Rigel code.
bool RigelTargetMachine::
addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel) 
{
  PM.add(createRigelISelDag(*this));
  return false;
}

// Implemented by targets that want to run passes immediately before 
// machine code is emitted. return true if -print-machineinstrs should 
// print out the code after the passes.
bool RigelTargetMachine::
addPreEmitPass(PassManagerBase &PM, CodeGenOpt::Level OptLevel) 
{
  return true;
}
