//===- RigelRegisterInfo.h - Rigel Register Information Impl ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Rigel implementation of the MRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef RIGELREGISTERINFO_H
#define RIGELREGISTERINFO_H

#include "Rigel.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "RigelGenRegisterInfo.h.inc"

namespace llvm {
class RigelSubtarget;
class TargetInstrInfo;
class Type;

struct RigelRegisterInfo : public RigelGenRegisterInfo {
  const RigelSubtarget &Subtarget;
  const TargetInstrInfo &TII;
  
  RigelRegisterInfo(const RigelSubtarget &Subtarget, const TargetInstrInfo &tii);

  /// getRegisterNumbering - Given the enum value for some register, e.g.
  /// Rigel::RA, return the number that it corresponds to (e.g. 31).
  static unsigned getRegisterNumbering(unsigned RegEnum);

  /// Adjust the stack frame.
  void adjustRigelStackFrame(MachineFunction &MF) const;

  /// Code generation virtual methods
  const unsigned *getCalleeSavedRegs(const MachineFunction* MF = 0) const;

  BitVector getReservedRegs(const MachineFunction &MF) const;

  bool hasFP(const MachineFunction &MF) const;

  void eliminateCallFramePseudoInstr(MachineFunction &MF,
                                     MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator I) const;

  /// Stack Frame Processing Methods
  void eliminateFrameIndex(MachineBasicBlock::iterator II,
                           int SPAdj, RegScavenger *RS = NULL) const;

  void processFunctionBeforeFrameFinalized(MachineFunction &MF) const;

  void emitPrologue(MachineFunction &MF) const;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const;
  
  /// Debug information queries.
  unsigned getRARegister() const;
  unsigned getFrameRegister(const MachineFunction &MF) const;

  /// Exception handling queries.
  unsigned getEHExceptionRegister() const;
  unsigned getEHHandlerRegister() const;

  int getDwarfRegNum(unsigned RegNum, bool isEH) const;
	// Acquire an unused register in an emergency.
  bool findScratchRegister(MachineBasicBlock::iterator II,
                               RegScavenger *RS,
                               const TargetRegisterClass *RC,
                               int SPAdj,
															 unsigned &ret) const;
};

} // end namespace llvm

#endif
