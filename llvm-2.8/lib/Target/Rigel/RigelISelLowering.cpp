//===-- RigelISelLowering.cpp - Rigel DAG Lowering Implementation -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Rigel uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "rigel-lower"
#include "RigelISelLowering.h"
#include "RigelMachineFunction.h"
#include "RigelTargetMachine.h"
//#include "RigelTargetObjectFile.h"
#include "RigelSubtarget.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Function.h"
#include "llvm/GlobalVariable.h"
#include "llvm/Intrinsics.h"
#include "llvm/CallingConv.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
using namespace llvm;


RigelTargetLowering::
RigelTargetLowering(RigelTargetMachine &TM)
  : TargetLowering(TM, new TargetLoweringObjectFileELF()) {

  Subtarget = TM.getSubtargetImpl() ;

  // The result of our SETCC operations is 0 or 1.
  // Other targets use 0 or -1, or "only bit 0 counts,
  // the rest are undefined"
  setBooleanContents(ZeroOrOneBooleanContent);

  addRegisterClass(MVT::i32, Rigel::CPURegsRegisterClass);
  addRegisterClass(MVT::f32, Rigel::FPRegsRegisterClass);

	setOperationAction(ISD::ConstantFP, MVT::f32, Expand);
	setOperationAction(ISD::FNEG, MVT::f32, Expand);

	setOperationAction(ISD::BIT_CONVERT, MVT::f32, Expand);
	setOperationAction(ISD::BIT_CONVERT, MVT::i32, Expand);
	setOperationAction(ISD::FSIN , MVT::f32, Expand);
	setOperationAction(ISD::FCOPYSIGN, MVT::f32, Expand);
	setOperationAction(ISD::FCOS , MVT::f32, Expand);
	setOperationAction(ISD::FREM , MVT::f32, Expand);
	setOperationAction(ISD::FSQRT, MVT::f32, Legal); //frsq->frcp
  //TODO is the frsq->frcp pattern accurate enough, or do we need to use the libcall ('Expand')?

  // Custom
  setOperationAction(ISD::GlobalAddress,      MVT::i32,   Custom);
  setOperationAction(ISD::BlockAddress,       MVT::i32,   Custom);
  setOperationAction(ISD::GlobalTLSAddress,   MVT::i32,   Custom);
  setOperationAction(ISD::JumpTable,          MVT::i32,   Custom);
  setOperationAction(ISD::ConstantPool,       MVT::i32,   Custom);

  setOperationAction(ISD::VASTART,            MVT::Other, Custom);

  //Rigel doesn't yet implement add/sub-with-carry, so expand it out (see BZ Bug 9)
  setOperationAction(ISD::ADDC, MVT::i32, Expand);
  setOperationAction(ISD::ADDE, MVT::i32, Expand);
  setOperationAction(ISD::SUBC, MVT::i32, Expand);
  setOperationAction(ISD::SUBE, MVT::i32, Expand);

  // Extended load operations for i1 types must be promoted 
  setLoadExtAction(ISD::EXTLOAD,  MVT::i1,  Promote);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i1,  Promote);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i1,  Promote);

  AddPromotedToType(ISD::SETCC, MVT::i1, MVT::i32);

  // Rigel doesn't have integrate jump-table or branch-on-CC instructions.
  setOperationAction(ISD::BR_JT,     MVT::Other, Expand);
  setOperationAction(ISD::BR_CC,     MVT::Other, Expand);

  setOperationAction(ISD::SELECT_CC, MVT::Other, Expand);
  setOperationAction(ISD::SETCC, MVT::i64, Expand);

  setOperationAction(ISD::SELECT,    MVT::i32,   Custom);
  setOperationAction(ISD::SELECT,    MVT::f32,   Custom);

  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i8, Legal); //sext8 instr
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i16, Legal); //sext16 instr

  //Expand indirect branches
  //setOperationAction(ISD::BRIND, MVT::Other, Expand);

  //FIXME Can we make any of these Legal?
  setOperationAction(ISD::MULHS, MVT::i32, Expand);
  setOperationAction(ISD::MULHU, MVT::i32, Expand);
  setOperationAction(ISD::UDIV, MVT::i32, Expand);
  setOperationAction(ISD::SDIV, MVT::i32, Expand);
  setOperationAction(ISD::SREM, MVT::i32, Expand);
  setOperationAction(ISD::UREM, MVT::i32, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i32, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i32, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i32, Expand);

  setOperationAction(ISD::CTPOP, MVT::i32, Expand);
  setOperationAction(ISD::CTTZ , MVT::i32, Expand);
  setOperationAction(ISD::ROTL , MVT::i32, Expand);
  setOperationAction(ISD::ROTR , MVT::i32, Expand);

  //TODO Implement memory barrier instruction like SPARCv9's
  //with a 4-bit immediate, with one bit each for
  //{Ld,St}->{Ld,St} barriers.
  setOperationAction(ISD::MEMBARRIER, MVT::Other, Expand);
  setOperationAction(ISD::BSWAP, MVT::i32, Expand);

  setOperationAction(ISD::SHL_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRA_PARTS, MVT::i32, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i32, Expand);

  // VASTART needs to be custom lowered to use the VarArgsFrameIndex.
  setOperationAction(ISD::VASTART           , MVT::Other, Custom);
  // Use default expansion for the other varargs SDNodes
  setOperationAction(ISD::VAARG             , MVT::Other, Expand);
  setOperationAction(ISD::VACOPY            , MVT::Other, Expand);
  setOperationAction(ISD::VAEND             , MVT::Other, Expand);

	//Rigel's F2I and I2F instructions cover si32->f32 and f32->si32.
	//We need to lower ui32->f32, si32->f64, and f32->ui32 to libcalls.
  //The latter is handled by 'Expand', the former 2 by 'Custom' code
  //in LowerINT_TO_FP() below.
	setOperationAction(ISD::SINT_TO_FP, MVT::i32, Custom);
  setOperationAction(ISD::UINT_TO_FP, MVT::i32, Custom);
	setOperationAction(ISD::FP_TO_SINT, MVT::i32, Legal);
  setOperationAction(ISD::FP_TO_UINT, MVT::i32, Expand);

  // Rigel subword loads and stores have to be custom lowered
  // FIXME Take into account subtargets with byte ld/st
  setOperationAction(ISD::LOAD,   MVT::i8, Custom);
  setOperationAction(ISD::LOAD,   MVT::i16, Custom);
  setOperationAction(ISD::STORE,  MVT::i8, Custom);
  setOperationAction(ISD::STORE,  MVT::i16, Custom);
  setLoadExtAction(ISD::EXTLOAD,  MVT::i8, Custom);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i8, Custom);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i8, Custom);
  setLoadExtAction(ISD::EXTLOAD,  MVT::i16, Custom);
  setLoadExtAction(ISD::ZEXTLOAD, MVT::i16, Custom);
  setLoadExtAction(ISD::SEXTLOAD, MVT::i16, Custom);

	//Need to handle unaligned i32 ld/st specially, others can be lowered normally.
  setOperationAction(ISD::LOAD, MVT::i32, Custom);
  setOperationAction(ISD::STORE, MVT::i32, Custom);
	setLoadExtAction(ISD::EXTLOAD, MVT::i32, Custom); //These shouldn't occur in practice,
	setLoadExtAction(ISD::SEXTLOAD, MVT::i32, Custom);//since our registers are 32-bit.
	setLoadExtAction(ISD::ZEXTLOAD, MVT::i32, Custom);

  setTruncStoreAction(MVT::i32, MVT::i8, Custom);
  setTruncStoreAction(MVT::i32, MVT::i16, Custom);
  setTruncStoreAction(MVT::i16, MVT::i8, Custom);

//Atomics
//FIXME Can you really just Promote i8 and i6 versions to i32?
//Seems like, if we don't support them in hardware, we need to wrap
//them in ldl/stc (which doesn't exist at the G$ yet) or do something
//else (grab some spinlock) to ensure atomicity.
//FIXME My first implementation of these will use the Rigel atomics
//that complete at the global cache.  What do these do if the value
//is cached in one or more L1's or L2's?  Is there any way we can detect
//at compile time whether anybody uses local ld/st on a location acted
//upon by these atomics (which would be an error)?  Can we somehow
//globally invalidate the value in all caches to make sure everybody
//gets the new value?  How do the atomics currently work under HW
//coherence?
    // setOperationAction(ISD::ATOMIC_CMP_SWAP,  MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_CMP_SWAP,  MVT::i16, Promote);
    setOperationAction(ISD::ATOMIC_CMP_SWAP,  MVT::i32, Legal);
    // setOperationAction(ISD::ATOMIC_SWAP,      MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_SWAP,      MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_SWAP,      MVT::i32, Custom);
    // setOperationAction(ISD::ATOMIC_LOAD_ADD,  MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_ADD,  MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_ADD,  MVT::i32, Custom);
    // setOperationAction(ISD::ATOMIC_LOAD_SUB,  MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_SUB,  MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_SUB,  MVT::i32, Custom);
    // setOperationAction(ISD::ATOMIC_LOAD_AND,  MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_AND,  MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_AND,  MVT::i32, Custom);
    // setOperationAction(ISD::ATOMIC_LOAD_OR,   MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_OR,   MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_OR,   MVT::i32, Custom);
    // setOperationAction(ISD::ATOMIC_LOAD_XOR,  MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_XOR,  MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_XOR,  MVT::i32, Custom);
    // setOperationAction(ISD::ATOMIC_LOAD_NAND, MVT::i8,  Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_NAND, MVT::i16, Promote);
    // setOperationAction(ISD::ATOMIC_LOAD_NAND, MVT::i32, Custom);

  //For now, assume that all our hardware atomics implicitly form a full
  //memory barrier for a single thread, and we don't need calls to
  //__sync_synchronize() or memory barriers before and after the call.
  //TODO Is the above actually true?  We should figure out the semantics
  //of atomics as far as memory barriers go, and implement the appropriate
  //set of memory barriers to get correctness in spite of multiple outstanding
  //loads or stores per thread (OutRider, non-blocking-loads/OoO completion, etc.)
  setShouldFoldAtomicFences(true);

  setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
  setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i32, Expand);
  setStackPointerRegisterToSaveRestore(Rigel::SP);
  computeRegisterProperties();
}


const char *RigelTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) 
  {
    case RigelISD::JmpLink    : return "RigelISD::JmpLink";
    case RigelISD::Hi         : return "RigelISD::Hi";
    case RigelISD::Lo         : return "RigelISD::Lo";
    case RigelISD::GPRel      : return "RigelISD::GPRel";
    case RigelISD::Ret        : return "RigelISD::Ret";
    case RigelISD::CMov       : return "RigelISD::CMov";
    case RigelISD::SelectCC   : return "RigelISD::SelectCC";
    case RigelISD::FPSelectCC : return "RigelISD::FPSelectCC";
    case RigelISD::FPBrcond   : return "RigelISD::FPBrcond";
    case RigelISD::FPCmp      : return "RigelISD::FPCmp";
    default                  : return NULL;
  }
}


MVT::SimpleValueType RigelTargetLowering::getSetCCResultType(EVT VT) const {
  return MVT::i32;
}

//log2 of the byte alignment of functions
unsigned RigelTargetLowering::getFunctionAlignment(const Function *F) const {
  return 2;
}

//! Expand a library call into an actual call DAG node
  /*!
   \note
   This code is taken from SelectionDAGLegalize, since it is not exposed as
   part of the LLVM SelectionDAG API.
   */

namespace {
  SDValue
  ExpandLibCall(RTLIB::Libcall LC, SDValue Op, SelectionDAG &DAG,
                bool isSigned, SDValue &Hi, const RigelTargetLowering &TLI) {
    // The input chain to this libcall is the entry node of the function.
    // Legalizing the call will automatically add the previous call to the
    // dependence.
    SDValue InChain = DAG.getEntryNode();

    TargetLowering::ArgListTy Args;
    TargetLowering::ArgListEntry Entry;
    for (unsigned i = 0, e = Op.getNumOperands(); i != e; ++i) {
      EVT ArgVT = Op.getOperand(i).getValueType();
      const Type *ArgTy = ArgVT.getTypeForEVT(*DAG.getContext());
      Entry.Node = Op.getOperand(i);
      Entry.Ty = ArgTy;
      Entry.isSExt = isSigned;
      Entry.isZExt = !isSigned;
      Args.push_back(Entry);
    }
    SDValue Callee = DAG.getExternalSymbol(TLI.getLibcallName(LC),
                                           TLI.getPointerTy());

    // Splice the libcall in wherever FindInputOutputChains tells us to.
    const Type *RetTy =
                Op.getNode()->getValueType(0).getTypeForEVT(*DAG.getContext());
    std::pair<SDValue, SDValue> CallInfo =
            TLI.LowerCallTo(InChain, RetTy, isSigned, !isSigned, false, false,
                            0, TLI.getLibcallCallingConv(LC),
                            /*isTailCall=*/false,
                            /*isReturnValueUsed=*/true,
                            Callee, Args, DAG, Op.getDebugLoc());

    return CallInfo.first;
  }
}

/// Lower ISD::SINT_TO_FP, ISD::UINT_TO_FP for i32
/// signed i32->f32 passes through unchanged, unsigned i32->f32 or
/// any i32->f64 is expanded to a libcall.
static SDValue LowerINT_TO_FP(SDValue Op, SelectionDAG &DAG,
                              const RigelTargetLowering &TLI) {
  EVT OpVT = Op.getValueType();
  SDValue Op0 = Op.getOperand(0);
  EVT Op0VT = Op0.getValueType();
  bool isSigned = (Op.getOpcode() == ISD::SINT_TO_FP);

  //Let's call signed i32 's32' and unsigned i32 'u32'.
  //Convert s32->f64, u32->f32, and u32->f64 to libcalls 
  if ((isSigned && Op0VT == MVT::i32 && OpVT == MVT::f64) ||
			(!isSigned && Op0VT == MVT::i32 && (OpVT == MVT::f64 || OpVT == MVT::f32))) {
    RTLIB::Libcall LC = isSigned ? RTLIB::getSINTTOFP(Op0VT, OpVT)
                                 : RTLIB::getUINTTOFP(Op0VT, OpVT);
    assert(LC != RTLIB::UNKNOWN_LIBCALL && "Unexpected int-to-fp conversion!");
    SDValue Dummy;
    return ExpandLibCall(LC, Op, DAG, false, Dummy, TLI);
  }

  return Op;
}

SDValue RigelTargetLowering::
LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
  EVT VT = Op.getValueType();
  unsigned Opc = (unsigned) Op.getOpcode();
  switch (Opc) 
  {
    default: {
      llvm_unreachable("Unhandled SDValue in LowerOperation()");
    }
    case ISD::ConstantPool:       return LowerConstantPool(Op, DAG);
    case ISD::GlobalAddress:      return LowerGlobalAddress(Op, DAG);
    case ISD::GlobalTLSAddress:   return LowerGlobalTLSAddress(Op, DAG);
    case ISD::BlockAddress:       return LowerBlockAddress(Op, DAG);
    case ISD::JumpTable:          return LowerJumpTable(Op, DAG);
    case ISD::SELECT:             return LowerSELECT(Op, DAG);
    case ISD::VASTART:            return LowerVASTART(Op, DAG);
    case ISD::SINT_TO_FP:
    case ISD::UINT_TO_FP:         return LowerINT_TO_FP(Op, DAG, *this);
    //Short loads and loadexts
    case ISD::LOAD:
    case ISD::EXTLOAD:
    case ISD::SEXTLOAD:
    case ISD::ZEXTLOAD:           return LowerLOAD(Op, DAG);
    case ISD::STORE:              return LowerSTORE(Op, DAG);
  }
  return SDValue();
}

/// ReplaceNodeResults - Replace the results of node with an illegal result
/// type with new values built out of custom code.
void RigelTargetLowering::ReplaceNodeResults(SDNode *N,
		                                         SmallVectorImpl<SDValue>&Results,
																						 SelectionDAG &DAG) const {
  switch (N->getOpcode()) {
    default:
			DEBUG(N->dump(&DAG));
      llvm_unreachable("Unhandled node in ReplaceNodeResults()!");
	    return;
		case ISD::LOAD:
			break;
			LoadSDNode *LD = dyn_cast<LoadSDNode>(N);
      assert(ISD::isUNINDEXEDLoad(LD) && "Indexed load during type legalization");
      EVT NVT = getTypeToTransformTo(*DAG.getContext(), LD->getValueType(0));
      ISD::LoadExtType ExtType =
        ISD::isNON_EXTLoad(LD) ? ISD::EXTLOAD : LD->getExtensionType();
      DebugLoc dl = LD->getDebugLoc();
      SDValue Res = DAG.getExtLoad(ExtType, NVT, dl, LD->getChain(), LD->getBasePtr(),
                                   LD->getSrcValue(), LD->getSrcValueOffset(),
                                   LD->getMemoryVT(), LD->isVolatile(),
                                   LD->isNonTemporal(), LD->getAlignment());

      Results.push_back(Res.getValue(0));
      Results.push_back(Res.getValue(1));
      break;
  }
}

//===----------------------------------------------------------------------===//
//  Lowering helper functions
//===----------------------------------------------------------------------===//

// AddLiveIn - This helper function adds the specified physical register to the
// MachineFunction as a live in value.  It also creates a corresponding
// virtual register for it.
static unsigned
AddLiveIn(MachineFunction &MF, unsigned PReg, TargetRegisterClass *RC) 
{
  assert(RC->contains(PReg) && "Not the correct regclass!");
  unsigned VReg = MF.getRegInfo().createVirtualRegister(RC);
  MF.getRegInfo().addLiveIn(PReg, VReg);
  return VReg;
}

MachineBasicBlock *
RigelTargetLowering::EmitInstrWithCustomInserter(MachineInstr *MI,
                                                MachineBasicBlock *BB) const
{
  const TargetInstrInfo *TII = getTargetMachine().getInstrInfo();
  bool isFPCmp = false;
  DebugLoc dl = MI->getDebugLoc();

  switch (MI->getOpcode()) {
  default: assert(0 && "Unhandled Opcode in EmitInstrWithCustomInserter()");
  case Rigel::Select_FCC_to_i:
  case Rigel::Select_FCC_to_f:
    isFPCmp = true; // FALL THROUGH
  case Rigel::Select_CC_to_i:
  case Rigel::Select_CC_to_f:
  {
    // To "insert" a SELECT_CC instruction, we actually have to insert the
    // diamond control-flow pattern.  The incoming instruction knows the
    // destination vreg to set, the condition code register to branch on, the
    // true/false values to select between, and a branch opcode to use.
    const BasicBlock *LLVM_BB = BB->getBasicBlock();
    MachineFunction::iterator It = BB;
    ++It;  //Need to increment the iterator because most functions that take an
           //iterator insert the thing you're talking about *before* the iterator
           //position.

    //  thisMBB:
    //  ...
    //   TrueVal = ...
    //   setcc r1, r2, r3
    //   bNE   r1, r0, copy1MBB
    //   fallthrough --> copy0MBB
    MachineBasicBlock *thisMBB  = BB;
    MachineFunction *F = BB->getParent();
    MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
    MachineBasicBlock *sinkMBB  = F->CreateMachineBasicBlock(LLVM_BB);
    F->insert(It, copy0MBB);
    F->insert(It, sinkMBB);
    
    // Transfer the remainder of BB and its successor edges to sinkMBB.
    sinkMBB->splice(sinkMBB->begin(), BB,
                    llvm::next(MachineBasicBlock::iterator(MI)),
                    BB->end());
    sinkMBB->transferSuccessorsAndUpdatePHIs(BB);

    // Next, add the true and fallthrough blocks as its successors.
    BB->addSuccessor(copy0MBB);
    BB->addSuccessor(sinkMBB);

    // Emit the right instruction according to the type of the operands compared
    if (isFPCmp) {
        assert (0 && "FP Select_FCC Unimplemented\n");
    } else
        BuildMI(BB, dl, TII->get(Rigel::BNZ)).addReg(MI->getOperand(1).getReg())
        .addMBB(sinkMBB); //if (compare result != 0) goto sinkMBB;
    
    //  copy0MBB:
    //   %FalseValue = ...
    //   # fallthrough to sinkMBB
    BB = copy0MBB;
    
    // Update machine-CFG edges
    BB->addSuccessor(sinkMBB);
    
    //  sinkMBB:
    //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
    //  ...
    BB = sinkMBB;
    BuildMI(*BB, BB->begin(), dl,
            TII->get(Rigel::PHI), MI->getOperand(0).getReg())
            .addReg(MI->getOperand(3).getReg()).addMBB(copy0MBB)
            .addReg(MI->getOperand(2).getReg()).addMBB(thisMBB);
            
    MI->eraseFromParent();   // The pseudo instruction is gone now.
    return BB;
  }
  } //switch
}

//===----------------------------------------------------------------------===//
//  Misc Lower Operation implementation
//===----------------------------------------------------------------------===//

SDValue RigelTargetLowering::
LowerSELECT(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Cond  = Op.getOperand(0); 
  SDValue True  = Op.getOperand(1);
  SDValue False = Op.getOperand(2);
  DebugLoc dl = Op.getDebugLoc();

  // if the incoming condition comes from a integer compare, the select 
  // operation must be SelectCC or a conditional move if the subtarget 
  // supports it.
  if (Cond.getOpcode() != RigelISD::FPCmp) {
    //FIXME Re-enable the CMOV path once we figure out CMOV codegen
    //FIXME Our regfile is not split, this can all be simplified
    //if (!True.getValueType().isFloatingPoint()){
    //  DEBUG(errs()  << "bailing out because this select can be lowered to a CMOV\n");
    //  return Op;
    //}
    //else {
      return DAG.getNode(RigelISD::SelectCC, dl, True.getValueType(), 
                         Cond, True, False);
    //}
  }

  // if the incoming condition comes from fpcmp, the select
  // operation must use FPSelectCC.
  SDValue CCNode = Cond.getOperand(2);
  return DAG.getNode(RigelISD::FPSelectCC, dl, True.getValueType(), 
                     Cond, True, False, CCNode);
}


SDValue RigelTargetLowering::
LowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Chain = Op.getOperand(0);
  SDValue Size = Op.getOperand(1);
  DebugLoc dl = Op.getDebugLoc();

  // Get a reference from Rigel stack pointer
  SDValue StackPointer = DAG.getCopyFromReg(Chain, dl, Rigel::SP, MVT::i32);

  // Subtract the dynamic size from the actual stack size to
  // obtain the new stack size.
  SDValue Sub = DAG.getNode(ISD::SUB, dl, MVT::i32, StackPointer, Size);

  // The Sub result contains the new stack start address, so it 
  // must be placed in the stack pointer register.
  Chain = DAG.getCopyToReg(StackPointer.getValue(1), dl, Rigel::SP, Sub);
  
  // This node always has two return values: a new stack pointer 
  // value and a chain
  SDValue Ops[2] = { Sub, Chain };
  return DAG.getMergeValues(Ops, 2, dl);
}

SDValue RigelTargetLowering::LowerGlobalAddress(SDValue Op,
                                                SelectionDAG &DAG) const {
  // FIXME there isn't actually debug info here
  DebugLoc dl = Op.getDebugLoc();
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();

  assert(getTargetMachine().getRelocationModel() != Reloc::PIC_ &&
         "PIC not supported in LowerGlobalAddress()");
  SDVTList VTs = DAG.getVTList(MVT::i32);

  //FIXME Once PIC and $gp are supported, implement gp-relative
  //relocations if the global address is in a small section
  //to save an instruction or two.

  // %hi/%lo relocation
  SDValue GAHi = DAG.getTargetGlobalAddress(GV, dl, MVT::i32, 0,
                                            RigelII::MO_ABS_HI);
  SDValue GALo = DAG.getTargetGlobalAddress(GV, dl, MVT::i32, 0,
                                            RigelII::MO_ABS_LO);
  SDValue HiPart = DAG.getNode(RigelISD::Hi, dl, VTs, &GAHi, 1);
  SDValue Lo = DAG.getNode(RigelISD::Lo, dl, MVT::i32, GALo);
  return DAG.getNode(ISD::ADD, dl, MVT::i32, HiPart, Lo);
}

SDValue RigelTargetLowering::
LowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const
{
  assert(0 && "TLS not yet implemented for Rigel.");
  return SDValue(); // Not reached
}

SDValue RigelTargetLowering::LowerBlockAddress(SDValue Op,
                                              SelectionDAG &DAG) const {
  assert(getTargetMachine().getRelocationModel() != Reloc::PIC_ &&
         "PIC not supported in LowerBlockAddress()");
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();
  // FIXME there isn't actually debug info here
  DebugLoc dl = Op.getDebugLoc();

  SDValue BAHi = DAG.getBlockAddress(BA, MVT::i32, true, RigelII::MO_ABS_HI);
  SDValue BALo = DAG.getBlockAddress(BA, MVT::i32, true, RigelII::MO_ABS_LO);
  SDValue Hi = DAG.getNode(RigelISD::Hi, dl, MVT::i32, BAHi);
  SDValue Lo = DAG.getNode(RigelISD::Lo, dl, MVT::i32, BALo);
  return DAG.getNode(ISD::ADD, dl, MVT::i32, Hi, Lo);
}

SDValue RigelTargetLowering::
LowerJumpTable(SDValue Op, SelectionDAG &DAG) const
{
  SDValue HiPart, JTI, JTILo;
  // FIXME there isn't actually debug info here
  DebugLoc dl = Op.getDebugLoc();
  bool IsPIC = getTargetMachine().getRelocationModel() == Reloc::PIC_;
  EVT PtrVT = Op.getValueType();
  JumpTableSDNode *JT = cast<JumpTableSDNode>(Op);

  assert(!IsPIC && "LowerJumpTable doesn't handle PIC yet");
  JTI = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, RigelII::MO_ABS_HI);
  HiPart = DAG.getNode(RigelISD::Hi, dl, PtrVT, JTI);
  JTILo = DAG.getTargetJumpTable(JT->getIndex(), PtrVT, RigelII::MO_ABS_LO);

  SDValue Lo = DAG.getNode(RigelISD::Lo, dl, PtrVT, JTILo);
  return DAG.getNode(ISD::ADD, dl, PtrVT, HiPart, Lo);
}

SDValue RigelTargetLowering::
LowerConstantPool(SDValue Op, SelectionDAG &DAG) const
{
  SDValue ResNode;
  ConstantPoolSDNode *N = cast<ConstantPoolSDNode>(Op);
  const Constant *C = N->getConstVal();
  // FIXME there isn't actually debug info here
  DebugLoc dl = Op.getDebugLoc();

  SDValue CPHi = DAG.getTargetConstantPool(C, MVT::i32, N->getAlignment(),
                                           N->getOffset(), RigelII::MO_ABS_HI);
  SDValue CPLo = DAG.getTargetConstantPool(C, MVT::i32, N->getAlignment(),
                                           N->getOffset(), RigelII::MO_ABS_LO);
  SDValue HiPart = DAG.getNode(RigelISD::Hi, dl, MVT::i32, CPHi);
  SDValue Lo = DAG.getNode(RigelISD::Lo, dl, MVT::i32, CPLo);
  ResNode = DAG.getNode(ISD::ADD, dl, MVT::i32, HiPart, Lo);
  return ResNode;
}

SDValue RigelTargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  RigelFunctionInfo *FuncInfo = MF.getInfo<RigelFunctionInfo>();

  DebugLoc dl = Op.getDebugLoc();
  SDValue FI = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(),
                                 getPointerTy());

  // vastart just stores the address of the VarArgsFrameIndex slot
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), dl, FI, Op.getOperand(1), SV, 0,
                      false, false, 0);
}

static bool
IsWordAlignedBasePlusConstantOffset(SelectionDAG &DAG, SDValue Addr, SDValue &AlignedBase,
                                    int64_t &Offset)
{

  if (Addr.getOpcode() != ISD::ADD) {
    return false;
  }
  ConstantSDNode *CN = 0;
  if (!(CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))) {
    return false;
  }
  //Don't change RigelHi/RigelLo pairs to avoid screwing up the assembler.
  //TODO Is there a way to tell the assembler "%lo(bla)-2"?
  if (Addr.getOperand(1).getOpcode() == RigelISD::Lo) {
    return false;
  }
  int64_t off = CN->getSExtValue();
  const SDValue &Base = Addr.getOperand(0);
  const SDValue *Root = &Base;
  if (Base.getOpcode() == ISD::ADD &&
      Base.getOperand(1).getOpcode() == ISD::SHL) {
    ConstantSDNode *CN = dyn_cast<ConstantSDNode>(Base.getOperand(1)
                                                      .getOperand(1));
    if (CN && (CN->getSExtValue() >= 2)) {
      Root = &Base.getOperand(0);
    }
  }
  if (isa<FrameIndexSDNode>(*Root)) {
    // All frame indices are word aligned (Rigel's ABI has >= 4 alignment for all datatypes)
    AlignedBase = Base;
    Offset = off;
    return true;
  }
  //TODO Other idioms on Rigel that are definitely word-aligned that LLVM doesn't already know about?
  return false;
}

SDValue RigelTargetLowering::LowerLOAD(SDValue Op, SelectionDAG &DAG) const {
  LoadSDNode *LN = cast<LoadSDNode>(Op);
  EVT InVT = LN->getMemoryVT();
  EVT OutVT = Op.getValueType();
  //FIXME Re-enable this once we are sure this function call will always
  //return false (since Rigel does not support any unaligned loads).
  //if (allowsUnalignedMemoryAccesses(LN->getMemoryVT())) {
  //  return SDValue();
  //}
  SDValue the_chain = LN->getChain();
  EVT PtrVT = DAG.getTargetLoweringInfo().getPointerTy();
  ISD::LoadExtType ExtType = LN->getExtensionType();
  unsigned alignment = LN->getAlignment();
  DebugLoc dl = Op.getDebugLoc();
  SDValue result;
  SDValue basePtr = LN->getBasePtr();
  SDValue shift;
  SDValue base;
  int64_t offset;

  if(LN->getAddressingMode() != ISD::UNINDEXED) {
  report_fatal_error("LowerLOAD: Got a LoadSDNode with an addr mode other "
                     "than UNINDEXED\n" +
                     Twine((unsigned)LN->getAddressingMode()));
  }

  if(InVT == MVT::i32) {
    if(alignment >= 4) {
      return SDValue();
    }
    //Let's statically lower to ld/ld/shr/shl/or if the address is base+offset
    //and it's not a RigelHi/RigelLo pair.  This way, we can elide the offset-
    //manipulation instructions in the libcall.
    //We'll get an unaligned load if we ever happen to have an unaligned base
    //address.  In this case, we'll know that we have to be more conservative
    //and only do this optimization when we know the base pointer itself is aligned.
    else if (IsWordAlignedBasePlusConstantOffset(DAG, basePtr, base, offset)) {
      if(offset % 4 == 0) { //Turns out to be word-aligned
        result = DAG.getLoad(MVT::i32, dl, the_chain, basePtr,
                       LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)),
                       LN->isVolatile(), LN->isNonTemporal(), 4);
        //TODO The offset may be 8-, 16-, etc. byte aligned.
        // We should provide that maximum known alignment to getLoad(), in case
        // we ever have wider loads in the future.

        // Update the chain
        the_chain = result.getValue(1);
      }
      else {
        SDValue lowOffset = DAG.getConstant(offset & ~0x3, MVT::i32);
        SDValue highOffset = DAG.getConstant((offset & ~0x3) + 4, MVT::i32);
        SDValue lowShift = DAG.getConstant((offset & 0x3) * 8, MVT::i32);
        SDValue highShift = DAG.getConstant(32 - ((offset & 0x3) * 8), MVT::i32);
        
        SDValue lowAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, base, lowOffset);
        SDValue highAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, base, highOffset);
        
        SDValue low = DAG.getLoad(getPointerTy(), dl, the_chain,
                                  lowAddr, LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)), LN->isVolatile(), LN->isNonTemporal(), 4); //TODO provide greater alignment if known
        SDValue high = DAG.getLoad(getPointerTy(), dl, the_chain,
                                   highAddr, LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)) + 4, LN->isVolatile(), LN->isNonTemporal(), 4); //TODO provide greater alignment if known
        SDValue lowShifted = DAG.getNode(ISD::SRL, dl, MVT::i32, low, lowShift);
        SDValue highShifted = DAG.getNode(ISD::SHL, dl, MVT::i32, high, highShift);
        result = DAG.getNode(ISD::OR, dl, MVT::i32, lowShifted, highShifted);
        the_chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, low.getValue(1),
                                high.getValue(1));
      }
    }
    else {
      // Lower to a call to __misaligned_load(BasePtr).
      const Type *IntPtrTy = getTargetData()->getIntPtrType(*DAG.getContext());
      TargetLowering::ArgListTy Args;
      TargetLowering::ArgListEntry Entry;
      
      Entry.Ty = IntPtrTy;
      Entry.Node = basePtr;
      Args.push_back(Entry);
      
      std::pair<SDValue, SDValue> CallResult =
            LowerCallTo(the_chain, IntPtrTy, false, false,
                        false, false, 0, CallingConv::C, false,
                        /*isReturnValueUsed=*/true,
                        DAG.getExternalSymbol("__unalignedldi32", getPointerTy()),
                        Args, DAG, dl);

      result = CallResult.first;
      the_chain = CallResult.second;
    }
  }
  else if(InVT == MVT::i16) {
    if(alignment >= 4) {
      result = DAG.getLoad(MVT::i32, dl, the_chain, basePtr,
                       LN->getSrcValue(), LN->getSrcValueOffset(),
                       LN->isVolatile(), LN->isNonTemporal(), 4);
      
      // Update the chain
      the_chain = result.getValue(1);
    }
    // If basePtr is base + offset, we assume base is word-aligned and we can simplify
    // because we know how the i16 falls within the 32-bit word(s).
    // TODO base may not be word-aligned; XCore searches further back in the DAG to
    // find a 'root' node that is definitely word-aligned, like a frameindex.  Rigel may
    // have other idioms that are definitely word-aligned.
    // We don't want to lower RigelHi/RigelLo pairs because that will interfere with
    // emitting the %hi/%lo directives if we're not careful.
    // If we're a constant offset from a globaladdress (add (add RigelHi, RigelLo), offset),
    // that could work.
    else if (IsWordAlignedBasePlusConstantOffset(DAG, basePtr, base, offset)) {
      if(offset % 4 == 0) { //Turns out to be word-aligned
        result = DAG.getLoad(MVT::i32, dl, the_chain, basePtr,
                       LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)),
                       LN->isVolatile(), LN->isNonTemporal(), 4);
        //TODO The offset may be 8-, 16-, etc. byte aligned.
        // We should provide that maximum known alignment to getLoad(), in case
        // we ever have wider loads in the future.

        // Update the chain
        the_chain = result.getValue(1);
      }
      else {
        int64_t offsetwithinword = offset & 0x3;
        if(offsetwithinword == 3) { //Crosses between words
          SDValue lowOffset = DAG.getConstant(offset & ~0x3, MVT::i32);
          SDValue highOffset = DAG.getConstant((offset & ~0x3) + 4, MVT::i32);
          SDValue lowShift = DAG.getConstant(24, MVT::i32);
          SDValue highShift = DAG.getConstant(8, MVT::i32);
          
          SDValue lowAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, base, lowOffset);
          SDValue highAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, base, highOffset);
          
          SDValue low = DAG.getLoad(getPointerTy(), dl, the_chain,
                                    lowAddr, LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)), LN->isVolatile(), LN->isNonTemporal(), 4); //TODO provide greater alignment if known
          SDValue high = DAG.getLoad(getPointerTy(), dl, the_chain,
                                     highAddr, LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)) + 4, LN->isVolatile(), LN->isNonTemporal(), 4); //TODO provide greater alignment if known
          SDValue lowShifted = DAG.getNode(ISD::SRL, dl, MVT::i32, low, lowShift);
          SDValue highShifted = DAG.getNode(ISD::SHL, dl, MVT::i32, high, highShift);
          result = DAG.getNode(ISD::OR, dl, MVT::i32, lowShifted, highShifted);
          the_chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, low.getValue(1),
                                  high.getValue(1));
          //NOTE: Because we don't zero out the top bits of highShifted before ORing, the top bits of result are garbage, so we *need* to sext/zext appropriately below.
        }
        else { //All within the same word
            SDValue lowOffset = DAG.getConstant(offset & ~0x3, MVT::i32);
            SDValue lowShift = DAG.getConstant((offset & 0x3) * 8, MVT::i32);
            SDValue lowAddr = DAG.getNode(ISD::ADD, dl, MVT::i32, base, lowOffset);
            SDValue low = DAG.getLoad(getPointerTy(), dl, the_chain,
                                      lowAddr, LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)), LN->isVolatile(), LN->isNonTemporal(), 4); //TODO provide greater alignment if known
            result = DAG.getNode(ISD::SRL, dl, MVT::i32, low, lowShift);
            the_chain = low.getValue(1);
        }
      }
    }
    else {
      // Lower to a call to __unalignedldi16(BasePtr).
      const Type *IntPtrTy = getTargetData()->getIntPtrType(*DAG.getContext());
      TargetLowering::ArgListTy Args;
      TargetLowering::ArgListEntry Entry;
      
      Entry.Ty = IntPtrTy;
      Entry.Node = basePtr;
      Args.push_back(Entry);
      
      std::pair<SDValue, SDValue> CallResult =
            LowerCallTo(the_chain, IntPtrTy, false, false,
                        false, false, 0, CallingConv::C, false,
                        /*isReturnValueUsed=*/true,
                        DAG.getExternalSymbol("__unalignedldi16", getPointerTy()),
                        Args, DAG, dl);

      result = CallResult.first;
      the_chain = CallResult.second;
      //NOTE: __unalignedldi16 returns a zero-extended i32, so if the load is a ZEXTLOAD or EXTLOAD, we don't need to do anything below.
      //TODO How to capture that and elide the zext below?
    }

    // Handle extending loads by extending the scalar result:
    if (ExtType == ISD::SEXTLOAD) {
      result = DAG.getNode(ISD::SIGN_EXTEND_INREG, dl, OutVT, result, DAG.getValueType(InVT));
    }
    //TODO is there a way to add an AssertZExt node to this?  Is it OK that ZERO_EXTEND probably returns an i32?
    else if (ExtType == ISD::ZEXTLOAD) {
      result = DAG.getNode(ISD::AND, dl, OutVT, result, DAG.getConstant(0xFFFF, OutVT));
    } else {
      if (OutVT.isFloatingPoint())
        result = DAG.getNode(ISD::FP_EXTEND, dl, OutVT, result); //FIXME this doesn't actually FP_EXTEND anything.  Figure out how we can do it if we need to support 16-bit floats (e.g., OpenCL)
      else {
        //If the load is anyext, we don't have to do anything; the high bits contain some garbage from a full-word load, but that's ok.
        result = DAG.getNode(ISD::AND, dl, OutVT, result, DAG.getConstant(0xFFFF, OutVT)); //FIXME shouldn't need anything here.
      }
    }
  }
  else if(InVT == MVT::i8) {
    SDValue bitoffset;
    if(alignment < 4)
    {
      //FIXME This seems unsafe.  It relies on our shamt encoding, and will break for 64-bit with 6-bit shamts.
      bitoffset = DAG.getNode(ISD::SHL, dl, MVT::i32, basePtr, DAG.getConstant(3, MVT::i32));
      basePtr = DAG.getNode(ISD::AND, dl, PtrVT, basePtr, DAG.getConstant(~(0x3), MVT::i32));
    }

    result = DAG.getLoad(MVT::i32, dl, the_chain, basePtr,
                         LN->getSrcValue(), (LN->getSrcValueOffset() & ~(0x3)),
                         LN->isVolatile(), LN->isNonTemporal(), 4);

    the_chain = result.getValue(1);

    if(alignment < 4)
    {
      result = DAG.getNode(ISD::SRL, dl, MVT::i32, result, bitoffset);
    }

    // Handle extending loads by extending the scalar result:
    if (ExtType == ISD::SEXTLOAD) {
      result = DAG.getNode(ISD::SIGN_EXTEND_INREG, dl, OutVT, result, DAG.getValueType(InVT));
    }
    //TODO is there a way to add an AssertZExt node to this?  Is it OK that ZERO_EXTEND probably returns an i32?
    else if (ExtType == ISD::ZEXTLOAD) {
      result = DAG.getNode(ISD::AND, dl, OutVT, result, DAG.getConstant(0xFF, OutVT));
    } else {
      if (OutVT.isFloatingPoint())
        result = DAG.getNode(ISD::FP_EXTEND, dl, OutVT, result); //FIXME this doesn't actually FP_EXTEND anything.  Figure out how we can do it if we need to support 16-bit floats (e.g., OpenCL)
      else {
        result = DAG.getNode(ISD::AND, dl, OutVT, result, DAG.getConstant(0xFF, OutVT)); //FIXME Shouldn't need anything here.
        //If the load is anyext, we don't have to do anything; the high bits contain some garbage from a full-word load, but that's ok.
      }
    }
  }
  else {
    //InVT != i8, i16, i32
    report_fatal_error("LowerLOAD: Got an InVT other than MVT::i{8,16,32}\n" +
                         Twine(InVT.getEVTString()));
  }
  SDValue retops[] = {
      result,
      the_chain
  };
  return DAG.getMergeValues(retops, 2, dl);
} //LowerLOAD

SDValue RigelTargetLowering::LowerSTORE(SDValue Op, SelectionDAG &DAG) const
{
  StoreSDNode *SN = cast<StoreSDNode>(Op);
  SDValue Value = SN->getValue();
  EVT VT = Value.getValueType();
  EVT StVT = (!SN->isTruncatingStore() ? VT : SN->getMemoryVT());
  EVT PtrVT = DAG.getTargetLoweringInfo().getPointerTy();
  DebugLoc dl = Op.getDebugLoc();
  unsigned alignment = SN->getAlignment();
  int src_value_offset = SN->getSrcValueOffset();
  //FIXME Re-enable this once we are sure this function call will always
  //return false (since Rigel does not support any unaligned loads).
  //if (allowsUnalignedMemoryAccesses(SN->getMemoryVT())) {
  //  return SDValue();
  //}
  SDValue basePtr = SN->getBasePtr();
  SDValue the_chain = SN->getChain();
  SDValue theValue = SN->getValue();

  if(SN->getAddressingMode() != ISD::UNINDEXED) {
    report_fatal_error("LowerSTORE: Got a StoreSDNode with an addr mode other "
                       "than UNINDEXED\n" +
                       Twine((unsigned)SN->getAddressingMode()));
  }

  if(StVT == MVT::i32) {
    if(alignment >= 4) {
      return SDValue();
    }
    else {
      // Lower to a call to __unalignedsti32(basePtr, theValue).
      const Type *IntPtrTy = getTargetData()->getIntPtrType(*DAG.getContext());
      TargetLowering::ArgListTy Args;
      TargetLowering::ArgListEntry Entry;
      
      Entry.Ty = IntPtrTy;
      Entry.Node = basePtr;
      Args.push_back(Entry);
      
      Entry.Ty = Type::getInt32Ty(*DAG.getContext());
      Entry.Node = theValue;
      Args.push_back(Entry);
      
      std::pair<SDValue, SDValue> CallResult =
            LowerCallTo(the_chain, Type::getVoidTy(*DAG.getContext()), false, false,
                        false, false, 0, CallingConv::C, false,
                        /*isReturnValueUsed=*/true,
                        DAG.getExternalSymbol("__unalignedsti32", getPointerTy()),
                        Args, DAG, dl);

      return CallResult.second; //return the chain
    }
  }
  else if(StVT == MVT::i16) {
    //FIXME Unfortunately, we cannot (easily) expand i16 and i8 stores inline
    //because there is an unspoken dependence on Rigel between all i16 and i8 stores
    //that is not captured in the default LLVM token chain generation framework.
    //Because we must do subword stores with word-level load/modify/store, we need
    //to ensure that the store of one i8 store, for example, comes before the load
    //of another.  This is very difficult or impossible in the current framework,
    //so we punt and call out to libcalls, which encapsulate the load and store nicely.
    //The #if 0 sections below represent a best effort attempt at being smart at generating
    //inline instruction sequences if we know something about the alignment of an i16/i8 store.
    //In the future, we can create different versions of the libcall with these same optimizations.
    //(Or more likely, this will all go away because we add byte and short loads and stores to avoid
    //all this nonsense)
    // Lower to a call to __unalignedsti16(basePtr, theValue).
    const Type *IntPtrTy = getTargetData()->getIntPtrType(*DAG.getContext());
    TargetLowering::ArgListTy Args;
    TargetLowering::ArgListEntry Entry;
    
    Entry.Ty = IntPtrTy;
    Entry.Node = basePtr;
    Args.push_back(Entry);
    
    Entry.Ty = Type::getInt16Ty(*DAG.getContext());
    Entry.Node = theValue;
    Args.push_back(Entry);
    
    std::pair<SDValue, SDValue> CallResult =
          LowerCallTo(the_chain, Type::getVoidTy(*DAG.getContext()), false, false,
                      false, false, 0, CallingConv::C, false,
                      /*isReturnValueUsed=*/true,
                      DAG.getExternalSymbol("__unalignedsti16", getPointerTy()),
                      Args, DAG, dl);

    return CallResult.second; //return the chain
  }
  else if(StVT == MVT::i8) {
    // FIXME See note above about i16 stores.  It applies here too.
    // Lower to a call to __sti8(basePtr, theValue).
    const Type *IntPtrTy = getTargetData()->getIntPtrType(*DAG.getContext());
    TargetLowering::ArgListTy Args;
    TargetLowering::ArgListEntry Entry;
    
    Entry.Ty = IntPtrTy;
    Entry.Node = basePtr;
    Args.push_back(Entry);
    
    Entry.Ty = Type::getInt8Ty(*DAG.getContext());
    Entry.Node = theValue;
    Args.push_back(Entry);
    
    std::pair<SDValue, SDValue> CallResult =
          LowerCallTo(the_chain, Type::getVoidTy(*DAG.getContext()), false, false,
                      false, false, 0, CallingConv::C, false,
                      /*isReturnValueUsed=*/true,
                      DAG.getExternalSymbol("__sti8", getPointerTy()),
                      Args, DAG, dl);

    return CallResult.second; //return the chain
  }
  else {
    //StVT != i8, i16, i32
    report_fatal_error("LowerSTORE: Got a StVT other than MVT::i{8,16,32}\n" +
                         Twine(StVT.getEVTString()));
  }
  llvm_unreachable("Called LowerSTORE() with an uncaught SDNode type!");
  return SDValue();
}

//===----------------------------------------------------------------------===//
//                  CALL Calling Convention Implementation
//===----------------------------------------------------------------------===//

#include "RigelGenCallingConv.inc"

/// CalculateStackSlotSize - Calculates the size reserved for this argument on
/// the stack.
static unsigned CalculateStackSlotSize(EVT ArgVT, ISD::ArgFlagsTy Flags,
                                       unsigned PtrByteSize) {
  unsigned ArgSize =ArgVT.getSizeInBits()/8;
  if (Flags.isByVal())
    ArgSize = Flags.getByValSize();
  ArgSize = ((ArgSize + PtrByteSize - 1)/PtrByteSize) * PtrByteSize;

  return ArgSize;
}

/// CALLSEQ_START and CALLSEQ_END are emitted.
/// TODO: isTailCall.
SDValue RigelTargetLowering::
LowerCall(SDValue Chain, SDValue Callee,
                              CallingConv::ID CallConv, bool isVarArg,
                              bool &isTailCall,
                              const SmallVectorImpl<ISD::OutputArg> &Outs,
                              const SmallVectorImpl<SDValue> &OutVals,
                              const SmallVectorImpl<ISD::InputArg> &Ins,
                              DebugLoc dl, SelectionDAG &DAG,
                              SmallVectorImpl<SDValue> &InVals) const {

  isTailCall = false;

  EVT PtrVT = DAG.getTargetLoweringInfo().getPointerTy();
  unsigned PtrByteSize = 4; 

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  bool IsPic = false;

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, getTargetMachine(), ArgLocs,
                 *DAG.getContext());

  // NOTE: Now we only allocate the 8 stack slots for the arg registers if the function is varargs.
  // MIPS o32 reserves the slots no matter what.  I'm hoping we can get away without it for non-varargs functions.
  if(isVarArg) {
    int VTsize = EVT(MVT::i32).getSizeInBits()/8;
    int firstFI = MFI->CreateFixedObject(VTsize, (VTsize*7), true); //Allocate an i32 stack slot 7 slots up to make sure we reserve 8 (not sure how this works).
  }

  CCInfo.AnalyzeCallOperands(Outs, CC_Rigel);
  
  // Count how many bytes are to be pushed on the stack, including the linkage
  // area, and parameter passing area.
  unsigned NumBytes = CCInfo.getNextStackOffset();
  Chain = DAG.getCALLSEQ_START(Chain, DAG.getIntPtrConstant(NumBytes, true));

  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;
  SmallVector<SDValue, 8> MemOpChains;

  // First/LastArgStackLoc contains the first/last 
  // "at stack" argument location.
  int LastArgStackLoc = 0;
  //NOTE We have 32 bytes of arg register spill space if the function is varargs.  See note above.
  unsigned FirstStackArgLoc = isVarArg ? 32 : 0;

  // Walk the register/memloc assignments, inserting copies/loads.
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];
    SDValue Arg = OutVals[i];
    
    // Promote the value if needed.
    switch (VA.getLocInfo()) {
    default: assert(0 && "Unknown loc info in LowerCall()");
    case CCValAssign::Full: break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, dl, VA.getLocVT(), Arg);
      break;
    }
    
    // Arguments that can be passed on register must be kept at 
    // RegsToPass vector
    if (VA.isRegLoc()) {
      RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
      continue;
    }
    
    // Register cant get to this point...
    assert(VA.isMemLoc());
    
    // Create the frame index object for this incoming parameter.
    LastArgStackLoc = (FirstStackArgLoc + VA.getLocMemOffset());
    int FI = MFI->CreateFixedObject(VA.getValVT().getSizeInBits()/8,
                                    LastArgStackLoc, true);

    SDValue PtrOff = DAG.getFrameIndex(FI,getPointerTy());

    // emit ISD::STORE whichs stores the 
    // parameter value to a stack Location
    MemOpChains.push_back(DAG.getStore(Chain, dl, Arg, PtrOff, NULL, 0,
                          false, false, 0));
  }

  // Create a TokenFactor to act as a successor to all store chain operands.
  // All the stores are independent from one another, but we need a barrier
  // TokenFactor to express dependencies between this group of stores and
  // later instructions.
  if (!MemOpChains.empty())     
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, 
                        &MemOpChains[0], MemOpChains.size());

  // Build a sequence of copy-to-reg nodes chained together with token 
  // chain and flag operands which copy the outgoing args into registers.
  // The InFlag in necessary since all emited instructions must be
  // stuck together.
  SDValue InFlag;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i) {
    Chain = DAG.getCopyToReg(Chain, dl, RegsToPass[i].first, 
                             RegsToPass[i].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // If the callee is a GlobalAddress/ExternalSymbol node (quite common, every
  // direct call is) turn it into a TargetGlobalAddress/TargetExternalSymbol 
  // node so that legalize doesn't hack it. 
  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee)) 
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), dl, getPointerTy());
  else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(S->getSymbol(), getPointerTy());

  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Flag);
  SmallVector<SDValue, 8> Ops;  // increased to handle 8 args in regs
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add argument registers to the end of the list so that they are 
  // known live into the call.
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
                                  RegsToPass[i].second.getValueType()));

  if (InFlag.getNode())
    Ops.push_back(InFlag);

  Chain  = DAG.getNode(RigelISD::JmpLink, dl, NodeTys, &Ops[0], Ops.size());
  InFlag = Chain.getValue(1);

  // Create a stack location to hold GP when PIC is used. This stack 
  // location is used on function prologue to save GP and also after all 
  // emited CALL's to restore GP. 
  assert(!IsPic && "PIC Not supported in LowerCALL()");  

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(NumBytes, true),
                             DAG.getIntPtrConstant(0, true), InFlag);
  InFlag = Chain.getValue(1);

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, isVarArg,
                         Ins, dl, DAG, InVals);
}

/// LowerCallResult - Lower the result values of an ISD::CALL into the
/// appropriate copies out of appropriate physical registers.  This assumes that
/// Chain/InFlag are the input chain/flag to use, and that TheCall is the call
/// being lowered. Returns a SDNode with the same number of values as the 
/// ISD::CALL.
SDValue RigelTargetLowering::
LowerCallResult(SDValue Chain, SDValue InFlag, 
        CallingConv::ID CallingConv, bool isVarArg,
	const SmallVectorImpl<ISD::InputArg> &Ins,
	DebugLoc dl, SelectionDAG &DAG,
	SmallVectorImpl<SDValue> &InVals) const {
  
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallingConv, isVarArg, getTargetMachine(), 
                 RVLocs, *DAG.getContext());

  CCInfo.AnalyzeCallResult(Ins, RetCC_Rigel);

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    Chain = DAG.getCopyFromReg(Chain, dl, RVLocs[i].getLocReg(),
                                 RVLocs[i].getValVT(), InFlag).getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Chain.getValue(0));
  }
  
  return Chain;
}

//===----------------------------------------------------------------------===//
//             FORMAL_ARGUMENTS Calling Convention Implementation
//===----------------------------------------------------------------------===//

/// LowerFORMAL_ARGUMENTS - transform physical registers into
/// virtual registers and generate load operations for
/// arguments places on the stack.
SDValue RigelTargetLowering::
LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::InputArg> &Ins,
                      DebugLoc dl, SelectionDAG &DAG,
                      SmallVectorImpl<SDValue> &InVals) const {

  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  RigelFunctionInfo *RigelFI = MF.getInfo<RigelFunctionInfo>();

  SmallVector<SDValue, 16> ArgValues;

  unsigned StackReg = MF.getTarget().getRegisterInfo()->getFrameRegister(MF);
  RigelFI->setVarArgsFrameIndex(0);

  // Used with vargs to acumulate store chains
  std::vector<SDValue> OutChains;

  // Keep track of last register used for arguments
  unsigned ArgRegEnd = 0;

  // FIXME When does GP need to be made a live-in?

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, getTargetMachine(), 
                 ArgLocs, *DAG.getContext());

  CCInfo.AnalyzeFormalArguments(Ins, CC_Rigel);
  SDValue StackPtr;

  //NOTE See Notes in LowerCALL() about trying to avoid allocating the 32 bytes of stack
  //space for non-vararg functions.
  unsigned FirstStackArgLoc = isVarArg ? 32 : 0;

  //FIXME Don't hardcode the number of regs we use to pass arguments, this should come from
  //the calling convention
  const unsigned Num_CC_Regs = 8;
  unsigned CCReg_Idx = 0;

  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i) {
    CCValAssign &VA = ArgLocs[i];

    // Arguments stored on registers
    if (VA.isRegLoc()) {
      EVT RegVT = VA.getLocVT();
      ArgRegEnd = VA.getLocReg();
      TargetRegisterClass *RC = 0;
            
      if (RegVT == MVT::i32 || RegVT == MVT::f32)
        RC = Rigel::CPURegsRegisterClass; 
      else
        assert(0 && "RegVT not supported by FORMAL_ARGUMENTS Lowering");

      // Transform the arguments stored on 
      // physical registers into virtual ones
      unsigned Reg = AddLiveIn(MF, ArgRegEnd, RC);
      SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);

      // Cast from f32 to i32
      if (RegVT == MVT::f32) {
        ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, MVT::i32);
        CCReg_Idx++;
        ArgValue = DAG.getNode(ISD::BIT_CONVERT, dl, MVT::f32, ArgValue);
      } else if (RegVT == MVT::i32) {
        CCReg_Idx++;
      }	else {
        assert(0 && "Unknown type in call arguments.");
      }
      
      // If this is an 8 or 16-bit value, it is really passed promoted 
      // to 32 bits.  Insert an assert[sz]ext to capture this, then 
      // truncate to the right size.
      if (VA.getLocInfo() == CCValAssign::SExt)
        ArgValue = DAG.getNode(ISD::AssertSext, dl, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      else if (VA.getLocInfo() == CCValAssign::ZExt)
        ArgValue = DAG.getNode(ISD::AssertZext, dl, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
      
      if (VA.getLocInfo() != CCValAssign::Full)
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);

      InVals.push_back(ArgValue);
    } else { // VA.isRegLoc()

      // sanity check
      assert(VA.isMemLoc());

      // The last arg is not a reg anymore
      ArgRegEnd = 0;
      
      // The stack pointer offset is relative to the caller stack frame. 
      // Since the real stack size is unknown here, a negative SPOffset 
      // is used so there's a way to adjust these offsets when the stack
      // size get known (on EliminateFrameIndex). A dummy SPOffset is 
      // used instead of a direct negative address (which is recorded to
      // be used on emitPrologue) to avoid mis-calc of the first stack 
      // offset on PEI::calculateFrameObjectOffsets.
      // Arguments are always 32-bit.
      unsigned ArgSize = VA.getLocVT().getSizeInBits()/8;
      int FI = MFI->CreateFixedObject(ArgSize, 0, true);
      RigelFI->recordLoadArgsFI(FI, -(ArgSize + 
        (FirstStackArgLoc + VA.getLocMemOffset())));

      // Create load nodes to retrieve arguments from the stack
      SDValue FIN = DAG.getFrameIndex(FI, getPointerTy());
      InVals.push_back(DAG.getLoad(VA.getValVT(), dl, Chain, FIN, NULL, 0,
                                   false, false, 0));
    }
  }

  // FIXME These should come from the calling convention, not here!
  // 32-bit registers used for parameter passing
  static const unsigned GPR[] = {           
		Rigel::A0, Rigel::A1, Rigel::A2, Rigel::A3,
		Rigel::A4, Rigel::A5, Rigel::A6, Rigel::A7,
  };
  const unsigned GPRLength = 8;

  // For returning structs by value, we copy the sret argument into $v0
  // and save the argument into a virtual register so that we can
  // access it from the caller after return.
  if (DAG.getMachineFunction().getFunction()->hasStructRetAttr()) {
    unsigned Reg = RigelFI->getSRetReturnReg();
    if (!Reg) {
      Reg = MF.getRegInfo().createVirtualRegister(getRegClassFor(MVT::i32));
      RigelFI->setSRetReturnReg(Reg);
    }
    SDValue Copy = DAG.getCopyToReg(DAG.getEntryNode(), dl, Reg, InVals[0]);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other, Copy, Chain);
  }

  // To meet ABI, when VARARGS are passed on registers, the registers
  // must have their values written to the caller stack frame. If the last
  // non-variadic argument was placed in the stack, there's no need to save any register. 
  if (isVarArg && ArgRegEnd){
    if (StackPtr.getNode() == 0)
      StackPtr = DAG.getRegister(StackReg, getPointerTy());
  
    //The last register argument that must be saved is Rigel::A7
    TargetRegisterClass *RC = Rigel::CPURegsRegisterClass;
    unsigned StackLoc = ArgLocs.size();

    for (++ArgRegEnd; ArgRegEnd <= Rigel::A7; ++ArgRegEnd, ++StackLoc) {
      unsigned Reg = AddLiveIn(DAG.getMachineFunction(), ArgRegEnd, RC); 
      SDValue ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, MVT::i32);

      int FI = MFI->CreateFixedObject(4, 0, true);
      RigelFI->recordStoreVarArgsFI(FI, -(4+(StackLoc*4)));
      SDValue PtrOff = DAG.getFrameIndex(FI, getPointerTy());
      OutChains.push_back(DAG.getStore(Chain, dl, ArgValue, PtrOff, NULL, 0,
                                       false, false, 0)); 

      // Record the frame index of the first variable argument
      // which is a value necessary to VASTART.
      if (!RigelFI->getVarArgsFrameIndex())
        RigelFI->setVarArgsFrameIndex(FI);
    }    
  }

  if (!OutChains.empty()) {
    OutChains.push_back(Chain);
    Chain = DAG.getNode(ISD::TokenFactor, dl, MVT::Other,
                        &OutChains[0],OutChains.size());
  }
  return Chain;
}


//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue RigelTargetLowering::
LowerReturn(SDValue Chain,
            CallingConv::ID CallConv, bool isVarArg,
            const SmallVectorImpl<ISD::OutputArg> &Outs,
            const SmallVectorImpl<SDValue> &OutVals,
            DebugLoc dl, SelectionDAG &DAG) const {

  // CCValAssign - represent the assignment of
  // the return value to a location
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, getTargetMachine(), 
                 RVLocs, *DAG.getContext());

  // Analize return values of ISD::RET
  CCInfo.AnalyzeReturn(Outs, RetCC_Rigel);

  // If this is the first return lowered for this function, add 
  // the regs to the liveout set for the function.
  if (DAG.getMachineFunction().getRegInfo().liveout_empty()) {
    for (unsigned i = 0; i != RVLocs.size(); ++i)
      if (RVLocs[i].isRegLoc())
        DAG.getMachineFunction().getRegInfo().addLiveOut(RVLocs[i].getLocReg());
  }

  SDValue Flag;

  // Copy the result values into the output registers.
  for (unsigned i = 0; i != RVLocs.size(); ++i) {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    SDValue Arg = OutVals[i];

    // Since the values are passed as i32's, need to do bit convert first
    if (Arg.getValueType() == MVT::f32) {
       Arg = DAG.getNode(ISD::BIT_CONVERT, dl, MVT::i32, Arg);
       Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), Arg, Flag);
    } else if (Arg.getValueType() == MVT::i32) {
       Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), Arg, Flag);
    } else {
      assert(0 && "Unknown value type for argument!");
    }

    // guarantee that all emitted copies are
    // stuck together, avoiding something bad
    Flag = Chain.getValue(1);
  }

  // For returning structs by value, we copy the sret argument into $v0.
  // We saved the argument into a virtual register in the entry block,
  // so now we copy the value out and into $v0.
  if (DAG.getMachineFunction().getFunction()->hasStructRetAttr()) {
    MachineFunction &MF      = DAG.getMachineFunction();
    RigelFunctionInfo *RigelFI = MF.getInfo<RigelFunctionInfo>();
    unsigned Reg = RigelFI->getSRetReturnReg();

    if (!Reg) 
      assert(0 && "sret virtual register not created in the entry block");
    SDValue Val = DAG.getCopyFromReg(Chain, dl, Reg, getPointerTy());

    Chain = DAG.getCopyToReg(Chain, dl, Rigel::V0, Val, Flag);
    Flag = Chain.getValue(1);
  }

  // Return on Rigel is always a "jmpr $ra"
  if (Flag.getNode())
    return DAG.getNode(RigelISD::Ret, dl, MVT::Other, 
                       Chain, DAG.getRegister(Rigel::RA, MVT::i32), Flag);
  else // Return Void
    return DAG.getNode(RigelISD::Ret, dl, MVT::Other, 
                       Chain, DAG.getRegister(Rigel::RA, MVT::i32));
}

//===----------------------------------------------------------------------===//
//                           Rigel Inline Assembly Support
//===----------------------------------------------------------------------===//

/// getRegClassForInlineAsmConstraint - Given a constraint letter (e.g. "r"),
/// return a list of registers that can be used to satisfy the constraint.
/// This should only be used for C_RegisterClass constraints.
std::pair<unsigned, const TargetRegisterClass*> RigelTargetLowering::
getRegForInlineAsmConstraint(const std::string &Constraint, EVT VT) const
{
  if (Constraint.size() == 1) {
    switch (Constraint[0]) {
    case 'r':
      return std::make_pair(0U, Rigel::CPURegsRegisterClass);
    }
  }
  return TargetLowering::getRegForInlineAsmConstraint(Constraint, VT);
}

/// Given a register class constraint, like 'r', if this corresponds directly
/// to an LLVM register class, return a register of 0 and the register class
/// pointer.
std::vector<unsigned> RigelTargetLowering::
getRegClassForInlineAsmConstraint(const std::string &Constraint,
                                  EVT VT) const
{
  if (Constraint.size() != 1)
    return std::vector<unsigned>();

  switch (Constraint[0]) {         
    default : break;
    case 'r':
      return make_vector<unsigned>(Rigel::T0, Rigel::T1, Rigel::T2, Rigel::T3, 
             Rigel::T4, Rigel::T5, Rigel::S0, Rigel::S1, 
             Rigel::S2, Rigel::S3, Rigel::S4, Rigel::S5, Rigel::S6, Rigel::S7, 
             0);
  }
  return std::vector<unsigned>();
}

//NOTE: If your target doesn't define this function, LLVM will fall back on the default
//one, which always allows offset folding under static relocation, never under PIC,
//and sometimes under dynamic-no-PIC.
//Allowing offset folding under static relocation caused bad code to be generated silently
//(no warnings/errors).  Specifically, references to struct members, particularly global/static global ones,
//would resolve to "ldw $rd, <pointer to struct>, ***0***, regardless of what the member's offset actually was.
//Another confounding bug is that we didn't explicitly set a default relocation model in RigelTargetMachine.cpp,
//we just left it as Reloc::Default if the user didn't pass one in.  By luck, that behavior also happened
//to disallow offset folding, and code would compile correctly if somehow we started with Reloc::Default.
//What triggered the bug was using Clang as a compiler driver, rather than just a cc1.  Clang as a compiler driver
//passes -relocation-model=static or -relocation-model=pic, depending on whether or not you pass it -fPIC.
bool RigelTargetLowering::isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const {
  // The Rigel target doesn't yet support offset folding
  return false;
}

//FIXME I tried defining this function and got a:
//fatal error: error in backend: Cannot yet select: 0x52a0c50: f32 = ConstantFP<0.000000e+00> [ORD=439] [ID=3]
//That doesn't seem right, since 0.000000e+00 should satisfy isZero(), but I'm taking this out for now because not having
//it never caused us problems.  I just added it at the same time as isOffsetFoldingLegal() because that was right next to it
//in MIPS and we didn't have it either.
//bool RigelTargetLowering::isFPImmLegal(const APFloat &Imm, EVT VT) const {
//  if (VT != MVT::f32 && VT != MVT::f64)
//    return false;
//  return Imm.isZero();
//}

bool RigelTargetLowering::allowsUnalignedMemoryAccesses(EVT VT) const {
  //No unaligned accesses yet
  return false;
}
