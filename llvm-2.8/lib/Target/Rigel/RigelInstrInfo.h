//===- RigelInstrInfo.h - Rigel Instruction Information -----------*- C++ -*-===//
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

#ifndef RIGELINSTRUCTIONINFO_H
#define RIGELINSTRUCTIONINFO_H

#include "Rigel.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "RigelRegisterInfo.h"

namespace llvm {

namespace Rigel {
  /// GetOppositeBranchOpcode - Return the inverse of the specified
  /// opcode, e.g. turning BEQ to BNE.
  unsigned GetOppositeBranchOpcode(unsigned Opcode);
} // namespace Rigel


  /// RigelII - This namespace holds all of the target specific flags that
  /// instruction info tracks.
  ///
  namespace RigelII {
    /// Target Operand Flag enum.
    enum TOF {
      //===------------------------------------------------------------------===//
      // Rigel Specific MachineOperand flags.

      MO_NO_FLAG,

      /// MO_GOT - Represents the offset into the global offset table at which
      /// the address the relocation entry symbol resides during execution.
      MO_GOT,

      /// MO_GOT_CALL - Represents the offset into the global offset table at 
      /// which the address of a call site relocation entry symbol resides 
      /// during execution. This is different from the above since this flag
      /// can only be present in call instructions.
      MO_GOT_CALL,

      /// MO_GPREL - Represents the offset from the current gp value to be used 
      /// for the relocatable object file being produced.
      MO_GPREL,

      /// MO_ABS_HI/LO - Represents the hi or low part of an absolute symbol
			/// address.
			MO_ABS_HI,
			MO_ABS_LO
    };
  }

class RigelInstrInfo : public TargetInstrInfoImpl {
  RigelTargetMachine &TM;
  const RigelRegisterInfo RI;
public:
  explicit RigelInstrInfo(RigelTargetMachine &TM);

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  virtual const RigelRegisterInfo &getRegisterInfo() const { return RI; }

  /// isLoadFromStackSlot - If the specified machine instruction is a direct
  /// load from a stack slot, return the virtual or physical register number of
  /// the destination along with the FrameIndex of the loaded stack slot.  If
  /// not, return 0.  This predicate must return 0 if the instruction has
  /// any side effects other than loading from the stack slot.
  virtual unsigned isLoadFromStackSlot(const MachineInstr *MI, int &FrameIndex) const;
  
  /// isStoreToStackSlot - If the specified machine instruction is a direct
  /// store to a stack slot, return the virtual or physical register number of
  /// the source reg along with the FrameIndex of the loaded stack slot.  If
  /// not, return 0.  This predicate must return 0 if the instruction has
  /// any side effects other than storing to the stack slot.
  virtual unsigned isStoreToStackSlot(const MachineInstr *MI, int &FrameIndex) const;
 
  /// Branch Analysis
  virtual bool AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                             MachineBasicBlock *&FBB,
                             SmallVectorImpl<MachineOperand> &Cond,
			     bool AllowModify) const;
  virtual unsigned RemoveBranch(MachineBasicBlock &MBB) const;
  virtual unsigned InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                                MachineBasicBlock *FBB,
                                const SmallVectorImpl<MachineOperand> &Cond,
				DebugLoc DL) const;

  virtual void copyPhysReg(MachineBasicBlock &MBB, 
                            MachineBasicBlock::iterator MI, DebugLoc DL,
                            unsigned DestReg, unsigned SrcReg,
	                    bool KillSrc) const;
  
  virtual void storeRegToStackSlot(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MBBI,
                                   unsigned SrcReg, bool isKill, int FrameIndex,
                                   const TargetRegisterClass *RC,
				   const TargetRegisterInfo *TRI) const;

  virtual void loadRegFromStackSlot(MachineBasicBlock &MBB,
                                    MachineBasicBlock::iterator MBBI,
                                    unsigned DestReg, int FrameIndex,
                                    const TargetRegisterClass *RC,
				    const TargetRegisterInfo *TRI) const;

  virtual bool ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const;

  /// Insert nop instruction when hazard condition is found
  virtual void insertNoop(MachineBasicBlock &MBB, 
                          MachineBasicBlock::iterator I) const;

  /// getGlobalBaseReg - Return a virtual register initialized with the
  /// the global base register value. Output instructions required to
  /// initialize the register in the function entry block, if necessary.
  unsigned getGlobalBaseReg(MachineFunction *MF) const;
}; // class RigelInstrInfo

} // namespace llvm

#endif
