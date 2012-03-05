//===-- RigelISelLowering.h - Rigel DAG Lowering Interface --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Rigel uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef RigelISELLOWERING_H
#define RigelISELLOWERING_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Target/TargetLowering.h"
#include "Rigel.h"
#include "RigelSubtarget.h"

namespace llvm {
  namespace RigelISD {
    enum NodeType {
      // Start the numbering from where ISD NodeType finishes.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,

      // Jump and link (call)
      JmpLink,

      // Get the Higher 16 bits from a 32-bit immediate
      // No relation with Rigel Hi register
      Hi, 

      // Get the Lower 16 bits from a 32-bit immediate
      // No relation with Rigel Lo register
      Lo, 

      // Handle gp_rel (small data/bss sections) relocation.
      GPRel,

      // Conditional Move
      CMov,

      // Select CC Pseudo Instruction
      SelectCC,

      // Floating Point Select CC Pseudo Instruction
      FPSelectCC,

      // Floating Point Branch Conditional
      FPBrcond,

      // Floating Point Compare
      FPCmp,

      // Return 
      Ret,
      RetNull,
      
      START_SPECIAL_OPS,
      // FP Ops
      FTOI,
      ITOF,
      LEA
    };
  }

  //===--------------------------------------------------------------------===//
  // TargetLowering Implementation
  //===--------------------------------------------------------------------===//
  class RigelTargetLowering : public TargetLowering 
  {
  public:

    explicit RigelTargetLowering(RigelTargetMachine &TM);

    /// LowerOperation - Provide custom lowering hooks for some operations.
    virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;

    /// getTargetNodeName - This method returns the name of a target specific 
    //  DAG node.
    virtual const char *getTargetNodeName(unsigned Opcode) const;

    /// ReplaceNodeResults - Replace the results of node with an illegal result
		/// type with new values built out of custom code.
		///
	  virtual void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue>&Results,
	                                  SelectionDAG &DAG) const;

    /// getSetCCResultType - get the ISD::SETCC result ValueType
    MVT::SimpleValueType getSetCCResultType(EVT VT) const;

    /// getFunctionAlignment - Return the Log2 alignment of this function.
    virtual unsigned getFunctionAlignment(const Function *F) const;
  private:
    // Subtarget Info
    const RigelSubtarget *Subtarget;

    // Lower Operand helpers
    virtual SDValue LowerCallResult(SDValue Chain, SDValue InFlag, 
                            CallingConv::ID CallingConv, bool isVarArg,
			    const SmallVectorImpl<ISD::InputArg> &Ins,
			    DebugLoc dl, SelectionDAG &DAG,
			    SmallVectorImpl<SDValue> &InVals) const;

    bool IsGlobalInSmallSection(GlobalValue *GV); 
    bool IsInSmallSection(unsigned Size); 

    // Lower Operand specifics
    SDValue LowerANDOR(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerJumpTable(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSELECT(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerLOAD(SDValue Op, SelectionDAG &DAG) const;
    SDValue LowerSTORE(SDValue Op, SelectionDAG &DAG) const;

    virtual SDValue LowerFormalArguments(SDValue Chain, 
			CallingConv::ID CallConv, bool isVarArg,
			const SmallVectorImpl<ISD::InputArg> &Ins,
    			DebugLoc dl, SelectionDAG &DAG, 
			SmallVectorImpl<SDValue> &InVals) const;

    virtual SDValue LowerCall(SDValue Chain, SDValue Callee,
                        CallingConv::ID CallConv, bool isVarArg,
			bool &isTailCall,
			const SmallVectorImpl<ISD::OutputArg> &Outs,
			const SmallVectorImpl<SDValue> &OutVals,
			const SmallVectorImpl<ISD::InputArg> &Ins,
			DebugLoc dl, SelectionDAG &DAG,
			SmallVectorImpl<SDValue> &InVals) const;

    virtual SDValue LowerReturn(SDValue Chain, 
			CallingConv::ID CallConv, bool isVarArg,
			const SmallVectorImpl<ISD::OutputArg> &Outs,
			const SmallVectorImpl<SDValue> &OutVals,
			DebugLoc dl, SelectionDAG &DAG) const;

    virtual MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr *MI,
                                                        MachineBasicBlock *MBB) const;

    std::pair<unsigned, const TargetRegisterClass*> 
              getRegForInlineAsmConstraint(const std::string &Constraint,
              EVT VT) const;

    std::vector<unsigned>
    getRegClassForInlineAsmConstraint(const std::string &Constraint,
              EVT VT) const;
    
    /// isOffsetFoldingLegal - Returns true if the target supports
    /// folding of struct/union member offsets.  See big long note in
    /// RigelISelLowering.h about the bug that results from not defining this
    /// function specifically for Rigel (since our backend is *not* offset-folding aware.
    virtual bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const;

    /// NOTE: Commented this function out due to error explained in this function's definition.
    /// isFPImmLegal - Returns true if the target can instruction select the
    /// specified FP immediate natively. If false, the legalizer will
    /// materialize the FP immediate as a load from a constant pool.
    //virtual bool isFPImmLegal(const APFloat &Imm, EVT VT) const;

    virtual bool allowsUnalignedMemoryAccesses(EVT VT) const;
  };
}

#endif // RigelISELLOWERING_H
