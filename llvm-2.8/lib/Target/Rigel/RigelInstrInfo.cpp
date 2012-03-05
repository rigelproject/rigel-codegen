//===- RigelInstrInfo.cpp - Rigel Instruction Information ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Rigel implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "rigel-instr-info"

#include "RigelInstrInfo.h"
#include "RigelTargetMachine.h"
#include "RigelMachineFunction.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "RigelGenInstrInfo.inc"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// TODO: Add subtarget support
RigelInstrInfo::RigelInstrInfo(RigelTargetMachine &tm)
  : TargetInstrInfoImpl(RigelInsts, array_lengthof(RigelInsts)),
    TM(tm), RI(*TM.getSubtargetImpl(), *this) {}

static bool isZeroImm(const MachineOperand &op) {
  return op.isImm() && op.getImm() == 0;
}

/// isLoadFromStackSlot - If the specified machine instruction is a direct
/// load from a stack slot, return the virtual or physical register number of
/// the destination along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than loading from the stack slot.
unsigned RigelInstrInfo::
isLoadFromStackSlot(const MachineInstr *MI, int &FrameIndex) const 
{
  if (MI->getOpcode() == Rigel::LW || MI->getOpcode() == Rigel::LWFP)
  {
    if ((MI->getOperand(2).isFI()) &&   // is a stack slot
        isZeroImm(MI->getOperand(1))) { // Offset is immediate 0
      FrameIndex = MI->getOperand(2).getIndex();
      return MI->getOperand(0).getReg();
    }
  }
  return 0;
}

/// isStoreToStackSlot - If the specified machine instruction is a direct
/// store to a stack slot, return the virtual or physical register number of
/// the source reg along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than storing to the stack slot.
unsigned RigelInstrInfo::
isStoreToStackSlot(const MachineInstr *MI, int &FrameIndex) const 
{
  switch(MI->getOpcode()) {
    case Rigel::SW:
    case Rigel::SWFP:
      if(((MI->getOperand(2).isFI()) && // is a stack slot
          (isZeroImm(MI->getOperand(1))))) { // the offset is a 0 immediate
        FrameIndex = MI->getOperand(2).getIndex();
        return MI->getOperand(0).getReg();
      }
      break;
    default:
      break;
  }
  return 0;
}

/// insertNoop - If data hazard condition is found insert the target nop
/// instruction.
void RigelInstrInfo::
insertNoop(MachineBasicBlock &MBB, MachineBasicBlock::iterator I) const 
{
  DebugLoc DL;
  if (I != MBB.end()) DL = I->getDebugLoc();
  BuildMI(MBB, I, DL, get(Rigel::NOP));
}

void RigelInstrInfo::
copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, DebugLoc DL,
             unsigned DestReg, unsigned SrcReg,
             bool KillSrc) const {
  BuildMI(MBB, I, DL, get(Rigel::OR), DestReg).addReg(Rigel::ZERO)
                      .addReg(SrcReg, getKillRegState(KillSrc));
  return ;
}

void RigelInstrInfo::
storeRegToStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
          unsigned SrcReg, bool isKill, int FI, 
          const TargetRegisterClass *RC,
	  const TargetRegisterInfo *TRI) const 
{
  DebugLoc DL;
  if (I != MBB.end()) DL = I->getDebugLoc();

  BuildMI(MBB, I, DL, get(Rigel::SW)).addReg(SrcReg, getKillRegState(isKill))
       .addImm(0).addFrameIndex(FI);
}

void RigelInstrInfo::
loadRegFromStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                     unsigned DestReg, int FI,
                     const TargetRegisterClass *RC,
		     const TargetRegisterInfo *TRI) const 
{

  DebugLoc DL;
  if (I != MBB.end()) DL = I->getDebugLoc();

  if (RC == Rigel::CPURegsRegisterClass || RC == Rigel::FPRegsRegisterClass)
    BuildMI(MBB, I, DL, get(Rigel::LW), DestReg).addImm(0).addFrameIndex(FI);
  else
    assert(0 && "Can't load this register from stack slot");
}

//===----------------------------------------------------------------------===//
// Branch Analysis
//===----------------------------------------------------------------------===//

//Can only analyze direct branches and jumps
static unsigned GetAnalyzableBrOpc(unsigned Opc) {
  //FIXME What about JAL?  Should JMP be here?
  return (Opc == Rigel::BEQ || Opc == Rigel::BNE || Opc == Rigel::BGE ||
          Opc == Rigel::BGT || Opc == Rigel::BLE || Opc == Rigel::BLT ||
          Opc == Rigel::BE  || Opc == Rigel::BNZ || Opc == Rigel::JMP)
    ? Opc : 0;
}

/// GetOppositeBranchOpcode - Return the inverse of the specified
/// opcode, e.g. turning BEQ to BNE.
unsigned Rigel::GetOppositeBranchOpcode(unsigned Opcode)
{
  switch (Opcode) {
  default:           llvm_unreachable("Illegal opcode!");
  case Rigel::BEQ:    return Rigel::BNE;
  case Rigel::BNE:    return Rigel::BEQ;
  case Rigel::BE:     return Rigel::BNZ;
  case Rigel::BNZ:    return Rigel::BE; 
  case Rigel::BGT:    return Rigel::BLE;
  case Rigel::BGE:    return Rigel::BLT;
  case Rigel::BLT:    return Rigel::BGE;
  case Rigel::BLE:    return Rigel::BGT;
  }
}

static void AnalyzeCondBr(const MachineInstr* Inst, unsigned Opc,
                          MachineBasicBlock *&BB,
                          SmallVectorImpl<MachineOperand>& Cond) {
  assert(GetAnalyzableBrOpc(Opc) && "Not an analyzable branch");
  int NumOp = Inst->getNumExplicitOperands();

  // for both int and fp branches, the last explicit operand is the
  // MBB.
  BB = Inst->getOperand(NumOp-1).getMBB();
  Cond.push_back(MachineOperand::CreateImm(Opc));

  for (int i=0; i<NumOp-1; i++)
    Cond.push_back(Inst->getOperand(i));
}

bool RigelInstrInfo::AnalyzeBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *&TBB,
                                   MachineBasicBlock *&FBB,
                                   SmallVectorImpl<MachineOperand> &Cond,
                                   bool AllowModify) const
{
  MachineBasicBlock::reverse_iterator I = MBB.rbegin(), REnd = MBB.rend();

  // Skip all the debug instructions.
  while (I != REnd && I->isDebugValue())
    ++I;

  if (I == REnd || !isUnpredicatedTerminator(&*I)) {
    // If this block ends with no branches (it just falls through to its succ)
    // just return false, leaving TBB/FBB null.
    TBB = FBB = NULL;
    return false;
  }

  MachineInstr *LastInst = &*I;
  unsigned LastOpc = LastInst->getOpcode();
  // Not an analyzable branch (must be an indirect jump).
  if (!GetAnalyzableBrOpc(LastOpc))
    return true;

  // Get the second to last instruction in the block.
  unsigned SecondLastOpc = 0;
  MachineInstr *SecondLastInst = NULL;

  if (++I != REnd) {
    SecondLastInst = &*I;
    SecondLastOpc = GetAnalyzableBrOpc(SecondLastInst->getOpcode());

    // Not an analyzable branch (must be an indirect jump).
    if (isUnpredicatedTerminator(SecondLastInst) && !SecondLastOpc)
      return true;
  }

  // If there is only one terminator instruction, process it.
  if (!SecondLastOpc) {
    // Unconditional branch
    if (LastOpc == Rigel::JMP) {
      TBB = LastInst->getOperand(0).getMBB();
      return false;
    }
    // Conditional branch
    AnalyzeCondBr(LastInst, LastOpc, TBB, Cond);
    return false;
  }

  // If we reached here, there are two branches.
  // If there are three terminators, we don't know what sort of block this is.
  if (++I != REnd && isUnpredicatedTerminator(&*I))
    return true;

  // If second to last instruction is an unconditional branch,
  // analyze it and remove the last instruction.
  if (SecondLastOpc == Rigel::JMP) {
    // Return if the last instruction cannot be removed.
    if (!AllowModify)
      return true;

    TBB = SecondLastInst->getOperand(0).getMBB();
    LastInst->eraseFromParent();
    return false;
  }

  // Conditional branch followed by an unconditional branch.
  // The last one must be unconditional.
  if (LastOpc != Rigel::JMP)
    return true;

  AnalyzeCondBr(SecondLastInst, SecondLastOpc, TBB, Cond);
  FBB = LastInst->getOperand(0).getMBB();

  return false;
}

unsigned RigelInstrInfo::
InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
             MachineBasicBlock *FBB,
             const SmallVectorImpl<MachineOperand> &Cond,
             DebugLoc DL) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");

  assert((Cond.size() == 3 || Cond.size() == 2 || Cond.size() == 0) &&
         "Rigel branch conditions can have zero|two|three components!");
  // # of condition operands:
  //  Unconditional branches: 0
  //  BranchZero: 2 (opc, reg)
  //  Branch: 3 (opc, reg0, reg1)

  if(FBB) { //Two-way branch
    //Build the conditional branch to TBB (True Basic Block)
    const unsigned Opcode = Cond[0].getImm();
    const TargetInstrDesc &TID = get(Opcode);
    if (TID.getNumOperands() == 3)
        BuildMI(&MBB, DL, TID).addReg(Cond[1].getReg())
                              .addReg(Cond[2].getReg())
                              .addMBB(TBB);
    else if(TID.getNumOperands() == 2)
        BuildMI(&MBB, DL, TID).addReg(Cond[1].getReg())
                              .addMBB(TBB);
    else
      llvm_unreachable("In 2-way InsertBranch(), TID didn't have 2 or 3 operands");
    //Build an unconditional branch to FBB (False Basic Block)
    BuildMI(&MBB, DL, get(Rigel::JMP)).addMBB(FBB);
    return 2;
  }
  else { //One-way branch
    if(Cond.empty()) //Unconditional branch
      BuildMI(&MBB, DL, get(Rigel::JMP)).addMBB(TBB);
    else { //Conditional branch
      const unsigned Opcode = Cond[0].getImm();
      const TargetInstrDesc &TID = get(Opcode);
      if (TID.getNumOperands() == 3)
        BuildMI(&MBB, DL, TID).addReg(Cond[1].getReg())
                              .addReg(Cond[2].getReg())
                              .addMBB(TBB);
      else if (TID.getNumOperands() == 2)
        BuildMI(&MBB, DL, TID).addReg(Cond[1].getReg())
                              .addMBB(TBB);
      else
        llvm_unreachable("In 1-way InsertBranch(), TID didn't have 2 or 3 operands");
    }
    return 1;
  }
}

unsigned RigelInstrInfo::
RemoveBranch(MachineBasicBlock &MBB) const
{
  MachineBasicBlock::reverse_iterator I = MBB.rbegin(), REnd = MBB.rend();
  MachineBasicBlock::reverse_iterator FirstBr;
  unsigned removed;

  // Skip all the debug instructions.
  while (I != REnd && I->isDebugValue())
    ++I;

  FirstBr = I;

  // Up to 2 branches are removed.
  // Note that indirect branches are not removed.
  for(removed = 0; I != REnd && removed < 2; ++I, ++removed)
    if (!GetAnalyzableBrOpc(I->getOpcode()))
      break;

  MBB.erase(I.base(), FirstBr.base());

  return removed;
}

/// ReverseBranchCondition - Invert the branch opcode in the
/// first element of the MachineOperand vector
bool RigelInstrInfo::
ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const 
{
  assert( (Cond.size() && Cond.size() <= 3) &&
          "Invalid Rigel branch condition!");
  Cond[0].setImm(Rigel::GetOppositeBranchOpcode(Cond[0].getImm()));
  return false;
}

/// getGlobalBaseReg - Return a virtual register initialized with the
/// the global base register value. Output instructions required to
/// initialize the register in the function entry block, if necessary.
//
unsigned RigelInstrInfo::getGlobalBaseReg(MachineFunction *MF) const {
  RigelFunctionInfo *RigelFI = MF->getInfo<RigelFunctionInfo>();
  unsigned GlobalBaseReg = RigelFI->getGlobalBaseReg();
  if (GlobalBaseReg != 0)
    return GlobalBaseReg;

  // Insert the set of GlobalBaseReg into the first MBB of the function
  MachineBasicBlock &FirstMBB = MF->front();
  MachineBasicBlock::iterator MBBI = FirstMBB.begin();
  MachineRegisterInfo &RegInfo = MF->getRegInfo();
  const TargetInstrInfo *TII = MF->getTarget().getInstrInfo();

  GlobalBaseReg = RegInfo.createVirtualRegister(Rigel::CPURegsRegisterClass);
  BuildMI(FirstMBB, MBBI, DebugLoc(), TII->get(TargetOpcode::COPY),
          GlobalBaseReg).addReg(Rigel::GP);
  RegInfo.addLiveIn(Rigel::GP);

  RigelFI->setGlobalBaseReg(GlobalBaseReg);
  return GlobalBaseReg;
}
