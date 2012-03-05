//===- RigelRegisterInfo.cpp - RIGEL Register Information -== -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the RIGEL implementation of the MRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "rigel-reg-info"

#include "Rigel.h"
#include "RigelSubtarget.h"
#include "RigelRegisterInfo.h"
#include "RigelMachineFunction.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineLocation.h"
#include "llvm/Target/TargetFrameInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"

using namespace llvm;

RigelRegisterInfo::RigelRegisterInfo(const RigelSubtarget &ST,
                                     const TargetInstrInfo &tii)
  : RigelGenRegisterInfo(Rigel::ADJCALLSTACKDOWN, Rigel::ADJCALLSTACKUP),
  Subtarget(ST), TII(tii) {}

/// getRegisterNumbering - Given the enum value for some register, e.g.
/// Rigel::RA, return the number that it corresponds to (e.g. 31).
unsigned RigelRegisterInfo::
getRegisterNumbering(unsigned RegEnum) 
{
  switch (RegEnum) {
    case Rigel::ZERO : return 0;
    case Rigel::AT   : return 1;
    case Rigel::V0   : return 2;
    case Rigel::V1   : return 3;
    case Rigel::A0   : return 4;
    case Rigel::A1   : return 5;
    case Rigel::A2   : return 6;
    case Rigel::A3   : return 7;
    case Rigel::A4   : return 8;
    case Rigel::A5   : return 9;
    case Rigel::A6   : return 10;
    case Rigel::A7   : return 11;
    case Rigel::T0   : return 12;
    case Rigel::T1   : return 13;
    case Rigel::T2   : return 14;
    case Rigel::T3   : return 15;
    case Rigel::T4   : return 16;
    case Rigel::T5   : return 17;
    case Rigel::S0   : return 18;
    case Rigel::S1   : return 19;
    case Rigel::S2   : return 20;
    case Rigel::S3   : return 21;
    case Rigel::S4   : return 22;
    case Rigel::S5   : return 23;
    case Rigel::S6   : return 24;
    case Rigel::S7   : return 25;
    case Rigel::K0   : return 26;
    case Rigel::K1   : return 27;
    case Rigel::GP   : return 28;
    case Rigel::SP   : return 29;
    case Rigel::FP   : return 30;
    case Rigel::RA   : return 31;
    default: assert(0 && "Unknown register number!");
  }    
}

//===----------------------------------------------------------------------===//
// Callee Saved Registers methods 
//===----------------------------------------------------------------------===//

/// Rigel Callee Saved Registers
const unsigned* RigelRegisterInfo::
getCalleeSavedRegs(const MachineFunction *MF) const 
{
  static const unsigned CalleeSavedRegs[] = {  
    Rigel::S0, Rigel::S1, Rigel::S2, Rigel::S3, 
    Rigel::S4, Rigel::S5, Rigel::S6, Rigel::S7, 0
  };
  return CalleeSavedRegs;
}

/// Rigel Callee Saved Register Classes
const TargetRegisterClass* const* 
RigelRegisterInfo::getCalleeSavedRegClasses(const MachineFunction *MF) const 
{
  static const TargetRegisterClass * const CalleeSavedRegClasses[] = {
    &Rigel::CPURegsRegClass, &Rigel::CPURegsRegClass,
    &Rigel::CPURegsRegClass, &Rigel::CPURegsRegClass,
    &Rigel::CPURegsRegClass, &Rigel::CPURegsRegClass,
    &Rigel::CPURegsRegClass, &Rigel::CPURegsRegClass, 0 
  };
  return CalleeSavedRegClasses;
}

BitVector RigelRegisterInfo::
getReservedRegs(const MachineFunction &MF) const
{
  BitVector Reserved(getNumRegs());
  Reserved.set(Rigel::ZERO);
  Reserved.set(Rigel::AT);
 // Reserved.set(Rigel::GP); //Testing out scavenging $GP
  Reserved.set(Rigel::SP);
  Reserved.set(Rigel::FP);
  Reserved.set(Rigel::RA);
  return Reserved;
}

//===----------------------------------------------------------------------===//
//
// Stack Frame Processing methods
// +----------------------------+
//
// The stack is allocated decrementing the stack pointer on
// the first instruction of a function prologue. Once decremented,
// all stack referencesare are done thought a positive offset
// from the stack/frame pointer, so the stack is considering
// to grow up! Otherwise terrible hacks would have to be made
// to get this stack ABI compliant :)
//
//  The stack frame required by the ABI:
//  Offset
//
//  0                 ----------
//  4                 Args to pass
//  .                 saved $GP  (used in PIC - not supported yet)
//  .                 Local Area
//  .                 saved "Callee Saved" Registers
//  .                 saved FP
//  .                 saved RA
//  StackSize         -----------
//
// Offset - offset from sp after stack allocation on function prologue
//
// The sp is the stack pointer subtracted/added from the stack size
// at the Prologue/Epilogue
//
// References to the previous stack (to obtain arguments) are done
// with offsets that exceeds the stack size: (stacksize+(4*(num_arg-1))
//
// Examples:
// - reference to the actual stack frame
//   for any local area var there is smt like : FI >= 0, StackOffset: 4
//     sw REGX, $sp, 4
//
// - reference to previous stack frame
//   suppose there's a load to the 5th arguments : FI < 0, StackOffset: 16.
//   The emitted instruction will be something like:
//     lw REGX, 16+StackSize(SP)
//
// Since the total stack size is unknown on LowerFORMAL_ARGUMENTS, all
// stack references (ObjectOffset) created to reference the function 
// arguments, are negative numbers. This way, on eliminateFrameIndex it's
// possible to detect those references and the offsets are adjusted to
// their real location.
//
//
//
//===----------------------------------------------------------------------===//

void RigelRegisterInfo::adjustRigelStackFrame(MachineFunction &MF) const
{
  MachineFrameInfo *MFI = MF.getFrameInfo();
  RigelFunctionInfo *RigelFI = MF.getInfo<RigelFunctionInfo>();
  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  unsigned StackAlign = MF.getTarget().getFrameInfo()->getStackAlignment();

  // See the description at RigelMachineFunction.h
  int TopCPUSavedRegOff = -1, TopFPUSavedRegOff = -1;

  // Replace the dummy '0' SPOffset by the negative offsets, as explained on 
  // LowerFORMAL_ARGUMENTS. Leaving '0' for while is necessary to avoid 
  // the approach done by calculateFrameObjectOffsets to the stack frame.
  RigelFI->adjustLoadArgsFI(MFI);
  RigelFI->adjustStoreVarArgsFI(MFI); 

  unsigned StackOffset = MFI->getStackSize();

  unsigned RegSize = 4;

  //NOTE: We used to have a problem where a subword stack object would be created,
  //the stack pointer would get misaligned, and you'd get unaligned load/store errors
  //in simulation.  To hack around that if it happens again, add something like:
  //StackOffset = (StackOffset + StackAlign - 1) & ~(StackAlign - 1);

  if (hasFP(MF)) {
    MFI->setObjectOffset(MFI->CreateStackObject(RegSize, RegSize, true), 
                         StackOffset);
    RigelFI->setFPStackOffset(StackOffset);
    TopCPUSavedRegOff = StackOffset;
    StackOffset += RegSize;
  }

  if (MFI->hasCalls()) {
    MFI->setObjectOffset(MFI->CreateStackObject(RegSize, RegSize, true), 
                         StackOffset);
    RigelFI->setRAStackOffset(StackOffset);
    TopCPUSavedRegOff = StackOffset;
    StackOffset += RegSize;
  }

  StackOffset = (StackOffset + StackAlign - 1) & ~(StackAlign - 1);
  
  // Update frame info
  MFI->setStackSize(StackOffset);

  if (TopCPUSavedRegOff >= 0)
    RigelFI->setCPUTopSavedRegOff(TopCPUSavedRegOff-StackOffset);

}

// hasFP - Return true if the specified function should have a dedicated frame
// pointer register.  This is true if the function has variable sized allocas or
// if frame pointer elimination is disabled.
bool RigelRegisterInfo::
hasFP(const MachineFunction &MF) const {
  return (NoFramePointerElim || MF.getFrameInfo()->hasVarSizedObjects());
}

// This function eliminate ADJCALLSTACKDOWN, 
// ADJCALLSTACKUP pseudo instructions
void RigelRegisterInfo::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  // Simply discard ADJCALLSTACKDOWN, ADJCALLSTACKUP instructions.
  MBB.erase(I);
}

// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void RigelRegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj, 
                    RegScavenger *RS) const 
{
DEBUG(errs()  << __FILE__ << "@" << __LINE__ << " Calling: " << __FUNCTION__ << "\n");
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();

  //TODO is this the correct DL?
  DebugLoc dl = II->getDebugLoc();

	// Find the FrameIndex
  unsigned FIOperandNum = 0;
  while (!MI.getOperand(FIOperandNum).isFI()) {
    ++FIOperandNum;
    assert(FIOperandNum < MI.getNumOperands() && "Instr doesn't have FrameIndex operand!");
  }

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int stackSize  = MFI->getStackSize();
  int spOffset   = MFI->getObjectOffset(FrameIndex);

  unsigned OffsetOperandNum;
  if (MI.isInlineAsm())
    OffsetOperandNum = FIOperandNum - 1;
  else
    OffsetOperandNum = (FIOperandNum == 2) ? 1 : 2;

  // as explained on LowerFORMAL_ARGUMENTS, detect negative offsets 
  // and adjust SPOffsets considering the final stack size.
  int Offset = ((spOffset < 0) ? (stackSize + (-(spOffset+4))) : (spOffset));
  Offset    += MI.getOperand(OffsetOperandNum).getImm();

  // Replace the FrameIndex with base register with (SP) or (FP).
  MI.getOperand(FIOperandNum).ChangeToRegister(getFrameRegister(MF),false);

  //Encode offsets in the 16-bit immediate field if we can
  if (isInt<16>(Offset)) {
    // Fits in immediate field
    MI.getOperand(OffsetOperandNum).ChangeToImmediate(Offset);
    return;
  } 

  MachineBasicBlock &MBB = *MI.getParent();

  // these 2 instrs get Offset into AT
  // FIXME do we need to use AT, or can we let the register allocator decide?
  BuildMI(MBB, II, dl, TII.get(Rigel::MVUi), Rigel::AT)
    .addImm(Offset >> 16);
  BuildMI(MBB, II, dl, TII.get(Rigel::ORi), Rigel::AT)
    .addReg(Rigel::AT, getKillRegState(true))
    .addImm(Offset & 0xFFFF);
  // add SP + Offset, result in AT
  BuildMI(MBB, II, dl, TII.get(Rigel::ADD), Rigel::AT)
    .addReg(Rigel::SP).addReg(Rigel::AT);

  // change base reg to AT from SP, AT now has the total of SP + Offset
  MI.getOperand(FIOperandNum).ChangeToRegister(Rigel::AT,false);
  MI.getOperand(OffsetOperandNum).ChangeToImmediate(0);
}

void RigelRegisterInfo::
emitPrologue(MachineFunction &MF) const 
{
  MachineBasicBlock &MBB   = MF.front();
  MachineFrameInfo *MFI    = MF.getFrameInfo();
  RigelFunctionInfo *RigelFI = MF.getInfo<RigelFunctionInfo>();
  MachineBasicBlock::iterator MBBI = MBB.begin();
  bool isPIC = false;    // FIXME Fill this in correctly to support PIC
  DebugLoc dl = MBBI->getDebugLoc();

  // Get the right frame order for Rigel
  adjustRigelStackFrame(MF);

  // Get the number of bytes to allocate from the FrameInfo.
  unsigned StackSize = MFI->getStackSize();

  // For simple functions, skip the prologue
  if (StackSize == 0 && !MFI->adjustsStack()) {
    return;
  }

  int FPOffset = RigelFI->getFPStackOffset();
  int RAOffset = RigelFI->getRAStackOffset();

  BuildMI(MBB, MBBI, dl, TII.get(Rigel::NOREORDER));
  
  // Adjust stack : addi sp, sp, (-imm)
  // Need to materialize the offset using MVUI+ORI if
  // stack size > 64k
  if (isInt<16>(StackSize)) {
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::ADDi), Rigel::SP)
      .addReg(Rigel::SP).addImm(-StackSize);
  } else {
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::MVUi), Rigel::AT)
      .addImm(StackSize >> 16);
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::ORi), Rigel::AT)
      .addReg(Rigel::AT, getKillRegState(true))
      .addImm(StackSize & 0xFFFF);
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::SUB), Rigel::SP)
      .addReg(Rigel::SP).addReg(Rigel::AT);
  }

  // Save the return address if this function isn't a leaf
  // sw  $ra, stack_loc($sp)
  if (MFI->hasCalls()) { 
    if (isInt<16>(RAOffset)){
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::SW))
        .addReg(Rigel::RA).addImm(RAOffset).addReg(Rigel::SP);
    } else {
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::MVUi), Rigel::AT)
        .addImm(RAOffset >> 16);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ORi), Rigel::AT)
        .addReg(Rigel::AT, getKillRegState(true))
        .addImm(RAOffset & 0xFFFF);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ADD), Rigel::AT)
        .addReg(Rigel::SP).addReg(Rigel::AT);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::SW))
        .addReg(Rigel::RA).addImm(0).addReg(Rigel::AT);
    }
  }

  // if framepointer enabled, save it and set it
  // to point to the stack pointer
  if (hasFP(MF)) {
    // sw  $fp,stack_loc($sp)
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::SW))
      .addReg(Rigel::FP).addImm(FPOffset).addReg(Rigel::SP);

    // move $fp, $sp
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::OR), Rigel::FP)
      .addReg(Rigel::SP).addReg(Rigel::ZERO);
  }
}

void RigelRegisterInfo::
emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const 
{
  MachineBasicBlock::iterator MBBI = prior(MBB.end());
  MachineFrameInfo *MFI            = MF.getFrameInfo();
  RigelFunctionInfo *RigelFI         = MF.getInfo<RigelFunctionInfo>();

  DebugLoc dl = MBBI->getDebugLoc();

  // Get the FI's where RA and FP are saved.
  int FPOffset = RigelFI->getFPStackOffset();
  int RAOffset = RigelFI->getRAStackOffset();

  // if framepointer enabled, restore it and restore the
  // stack pointer
  if (hasFP(MF)) {
    // move $sp, $fp
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::OR), Rigel::SP)
      .addReg(Rigel::FP).addReg(Rigel::ZERO);

    // lw  $fp,stack_loc($sp)
    BuildMI(MBB, MBBI, dl, TII.get(Rigel::LW))
      .addReg(Rigel::FP).addImm(FPOffset).addReg(Rigel::SP);
  }

  // Get the number of bytes from FrameInfo
  int StackSize = (int) MFI->getStackSize();

  if (StackSize == 0) {
    return;
  }

  // Restore the return address only if the function isnt a leaf one.
  // lw  $ra, stack_loc($sp)
  if (MFI->hasCalls()) { 
    if (isInt<16>(RAOffset)){
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::LW))
        .addReg(Rigel::RA).addImm(RAOffset).addReg(Rigel::SP);
    } else {
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::MVUi), Rigel::AT)
        .addImm(RAOffset >> 16);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ORi), Rigel::AT)
        .addReg(Rigel::AT, getKillRegState(true))
        .addImm(RAOffset & 0xFFFF);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ADD), Rigel::AT)
        .addReg(Rigel::SP).addReg(Rigel::AT);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::LW))
        .addReg(Rigel::RA).addImm(0).addReg(Rigel::AT);

    }
  }

  // adjust stack  : insert addi sp, sp, (imm)
  if (StackSize) {
    if (isInt<16>(StackSize)) {
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ADDi), Rigel::SP)
        .addReg(Rigel::SP).addImm(StackSize);
    } else {
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::MVUi), Rigel::AT)
        .addImm(StackSize >> 16);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ORi), Rigel::AT)
        .addReg(Rigel::AT, getKillRegState(true))
        .addImm(StackSize & 0xFFFF);
      BuildMI(MBB, MBBI, dl, TII.get(Rigel::ADD), Rigel::SP)
        .addReg(Rigel::SP).addReg(Rigel::AT);

    }
  }
}

void RigelRegisterInfo::
processFunctionBeforeFrameFinalized(MachineFunction &MF) const {
  MachineFrameInfo *MFI = MF.getFrameInfo();
  bool isPIC = (MF.getTarget().getRelocationModel() == Reloc::PIC_);
  if (MFI->hasCalls() && isPIC) {
    RigelFunctionInfo *RigelFI = MF.getInfo<RigelFunctionInfo>();
    MFI->setObjectOffset(RigelFI->getGPFI(), RigelFI->getGPStackOffset());
  }
}

unsigned RigelRegisterInfo::
getRARegister() const {
  return Rigel::RA;
}

unsigned RigelRegisterInfo::
getFrameRegister(const MachineFunction &MF) const {
  return hasFP(MF) ? Rigel::FP : Rigel::SP;
}

unsigned RigelRegisterInfo::
getEHExceptionRegister() const {
  assert(0 && "What is the exception register");
  return 0;
}

unsigned RigelRegisterInfo::
getEHHandlerRegister() const {
  assert(0 && "What is the exception handler register");
  return 0;
}

int RigelRegisterInfo::
getDwarfRegNum(unsigned RegNum, bool isEH) const {
  assert(0 && "What is the dwarf register number");
  return -1;
}

#include "RigelGenRegisterInfo.inc"
