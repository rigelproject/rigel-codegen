//===-- RigelExpandPseudoInsts.cpp - Expand pseudo instructions -----*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expand pseudo instructions into target
// instructions to allow proper scheduling, if-conversion, and other late
// optimizations. This pass should be run after register allocation but before
// post- regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "rigel-pseudo"
#include "Rigel.h"
#include "RigelInstrInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetRegisterInfo.h"
using namespace llvm;

namespace {
  class RigelExpandPseudo : public MachineFunctionPass {

  public:
    static char ID;
    RigelExpandPseudo() : MachineFunctionPass(ID) {}

    const TargetInstrInfo *TII;
    const TargetRegisterInfo *TRI;

    virtual bool runOnMachineFunction(MachineFunction &Fn);

    virtual const char *getPassName() const {
      return "Rigel pseudo instruction expansion pass";
    }

  private:
    void TransferImpOps(MachineInstr &OldMI,
                        MachineInstrBuilder &UseMI, MachineInstrBuilder &DefMI);
    bool ExpandMBB(MachineBasicBlock &MBB);
  };
  char RigelExpandPseudo::ID = 0;
}

/// TransferImpOps - Transfer implicit operands on the pseudo instruction to
/// the instructions created from the expansion.
void RigelExpandPseudo::TransferImpOps(MachineInstr &OldMI,
                                     MachineInstrBuilder &UseMI,
                                     MachineInstrBuilder &DefMI) {
  const TargetInstrDesc &Desc = OldMI.getDesc();
  for (unsigned i = Desc.getNumOperands(), e = OldMI.getNumOperands();
       i != e; ++i) {
    const MachineOperand &MO = OldMI.getOperand(i);
    assert(MO.isReg() && MO.getReg());
    if (MO.isUse())
      UseMI.addReg(MO.getReg(), getKillRegState(MO.isKill()));
    else
      DefMI.addReg(MO.getReg(),
                   getDefRegState(true) | getDeadRegState(MO.isDead()));
  }
}

bool RigelExpandPseudo::ExpandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineInstr &MI = *MBBI;
    MachineBasicBlock::iterator NMBBI = llvm::next(MBBI);

    bool ModifiedOp = true;
    unsigned Opcode = MI.getOpcode();
    switch (Opcode) {
    default:
      ModifiedOp = false;
      break;

    case Rigel::MOVi32imm: {
      unsigned DstReg = MI.getOperand(0).getReg();
      bool DstIsDead = MI.getOperand(0).isDead();
      const MachineOperand &MO = MI.getOperand(1);
      MachineInstrBuilder HI16, LO16;

      HI16 = BuildMI(MBB, MBBI, MI.getDebugLoc(),
                     TII->get(Rigel::MVUi),
                     DstReg);
      LO16 = BuildMI(MBB, MBBI, MI.getDebugLoc(),
                     TII->get(Rigel::ORi))
        .addReg(DstReg, getDefRegState(true) | getDeadRegState(DstIsDead))
        .addReg(DstReg);

      if (MO.isImm()) {
        unsigned Imm = MO.getImm();
        unsigned Lo16 = Imm & 0xffff;
        unsigned Hi16 = (Imm >> 16) & 0xffff;
        LO16 = LO16.addImm(Lo16);
        HI16 = HI16.addImm(Hi16);
      } else {
        const GlobalValue *GV = MO.getGlobal();
        unsigned TF = MO.getTargetFlags();
        LO16 = LO16.addGlobalAddress(GV, MO.getOffset(), TF | RigelII::MO_ABS_LO);
        HI16 = HI16.addGlobalAddress(GV, MO.getOffset(), TF | RigelII::MO_ABS_HI);
      }
      (*LO16).setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
      (*HI16).setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
      TransferImpOps(MI, HI16, LO16); //FIXME Not sure on the order we need to pass these.
      //ARM passed LO16, HI16, but they execute LO16 before HI16.
      MI.eraseFromParent();
      break;
    }
    }

    if (ModifiedOp)
      Modified = true;
    MBBI = NMBBI;
  }

  return Modified;
}

bool RigelExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  TII = MF.getTarget().getInstrInfo();
  TRI = MF.getTarget().getRegisterInfo();

  bool Modified = false;
  for (MachineFunction::iterator MFI = MF.begin(), E = MF.end(); MFI != E;
       ++MFI)
    Modified |= ExpandMBB(*MFI);
  return Modified;
}

/// createRigelExpandPseudoPass - returns an instance of the pseudo instruction
/// expansion pass.
FunctionPass *llvm::createRigelExpandPseudoPass() {
  return new RigelExpandPseudo();
}
