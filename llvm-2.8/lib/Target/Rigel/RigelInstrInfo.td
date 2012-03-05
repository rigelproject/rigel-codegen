//===- RigelInstrInfo.td - Rigel Instruction defs --------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Define instruction layouts
//===----------------------------------------------------------------------===//

include "RigelInstrFormats.td"

//===----------------------------------------------------------------------===//
// Rigel-specific SDTypeProfiles
//===----------------------------------------------------------------------===//
//takes {int, x, x}, produces {x}
def SDT_RigelSelectCC     : SDTypeProfile<1, 3, [SDTCisSameAs<0, 2>,
                            SDTCisSameAs<2, 3>, SDTCisInt<1>]>;
//takes {int, x, x, int}, produces {x}
def SDT_RigelFPSelectCC : SDTypeProfile<1, 4, [SDTCisInt<1>, SDTCisInt<4>,
                            SDTCisSameAs<0, 2>, SDTCisSameAs<2, 3>]>;
//takes {fp, int}, produces {fp}
def SDT_RigelFPCmp : SDTypeProfile<0, 3, [SDTCisSameAs<0, 1>, SDTCisFP<0>, 
                                          SDTCisInt<2>]>;
//takes {fp}, produces {i32}
def SDT_RigelFTOI : SDTypeProfile<1, 1, [SDTCisVT<0, i32>, SDTCisFP<1>]>;
//takes {i32}, produces {fp}
def SDT_RigelITOF : SDTypeProfile<1, 1, [SDTCisFP<0>, SDTCisVT<1, i32>]>;
//takes {int}, no outputs
//used to generate 'jmpr $ra', since Rigel doesn't have a 'ret' instruction
def SDT_RigelRet : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
//takes {ptr}, no outputs
def SDT_RigelJmpLink : SDTypeProfile<0, 1, [SDTCisPtrTy<0>]>;

//===----------------------------------------------------------------------===//
// Other Rigel-specific SelectionDAG Constructs
//===----------------------------------------------------------------------===//

// Need to tell LLVM what operands our CallSeqStart and CallSeqEnd take
def SDT_RigelCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>]>;
def SDT_RigelCallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;

//===----------------------------------------------------------------------===//
// Rigel-specific SDNodes
//===----------------------------------------------------------------------===//

def RigelFPCmp : SDNode<"RigelISD::FPCmp", SDT_RigelFPCmp>;
def RigelFTOI  : SDNode<"RigelISD::FTOI", SDT_RigelFTOI>;
def RigelITOF  : SDNode<"RigelISD::ITOF", SDT_RigelITOF>;
def RigelFTOI_Bitcast : SDNode<"RigelISD::FTOI_Bitcast", SDT_RigelFTOI>;
def RigelJmpLink : SDNode<"RigelISD::JmpLink",SDT_RigelJmpLink, 
                     [SDNPHasChain, SDNPOutFlag, SDNPOptInFlag, SDNPVariadic]>;
//These are used to refer to the top and bottom 16-bits of 32-bit constants,
//since most of our instructions only have 16-bit immediate fields
def RigelHi : SDNode<"RigelISD::Hi", SDTIntUnaryOp>;
def RigelLo : SDNode<"RigelISD::Lo", SDTIntUnaryOp>;

// Select between 2 values based on int or fp condition code from comparison op
def RigelSelectCC  : SDNode<"RigelISD::SelectCC", SDT_RigelSelectCC>;
def RigelFPSelectCC : SDNode<"RigelISD::FPSelectCC", SDT_RigelFPSelectCC>;

def RigelRet : SDNode<"RigelISD::Ret", SDT_RigelRet, 
                          [SDNPHasChain, SDNPOptInFlag]>;
def RigelRetNull : SDNode<"RigelISD::RetNull", SDTNone, 
		                      [SDNPHasChain, SDNPOptInFlag]>;

def callseq_start   : SDNode<"ISD::CALLSEQ_START", SDT_RigelCallSeqStart, 
                             [SDNPHasChain, SDNPOutFlag]>;
def callseq_end     : SDNode<"ISD::CALLSEQ_END", SDT_RigelCallSeqEnd, 
                             [SDNPHasChain, SDNPOptInFlag, SDNPOutFlag]>;

//===----------------------------------------------------------------------===//
// Rigel Instruction Operand Types
//===----------------------------------------------------------------------===//

// Instruction operand types
def brtarget    : Operand<OtherVT>;
def calltarget  : Operand<i32>;
def uimm16      : Operand<i32>;
def simm16      : Operand<i32>;
def shamt       : Operand<i32>; 
def condcode    : Operand<i32>;

// Tell LLVM how we want memory operands to be printed.  We use 'rs, rt, imm'.
// TODO Maybe we should move to a more common 'rs, (imm)rt' or similar
def mem : Operand<i32> {
  let PrintMethod = "printMemOperand";
  let MIOperandInfo = (ops simm16, CPURegs);
}

//===----------------------------------------------------------------------===//
// Rigel Leaf Patterns and Transformations
//===----------------------------------------------------------------------===//

// Get the 16 lsbits of an i32
def LO16 : SDNodeXForm<imm, [{
  return getI32Imm((unsigned)N->getZExtValue() & 0xFFFF);
}]>;

// Transformation Function - get the 16 msbits of an i32.
def HI16 : SDNodeXForm<imm, [{
  return getI32Imm(((unsigned)N->getZExtValue() >> 16) & 0xFFFF);
}]>;

// Turn an immediate into an immSExt16 if the value can be expressed
// as a sign-extended 16-bit immediate
// instructions like addi use sign-extended 16-bit immediates
// FIXME for immSExt16 and immZExt16, are MVT::i32 and MVT::i64 the
// only VTs we need to handle?
def immSExt16  : PatLeaf<(imm), [{
  if (N->getValueType(0) == MVT::i32)
    return (int32_t)N->getSExtValue() == (short)N->getSExtValue();
  else    
    return (int64_t)N->getSExtValue() == (short)N->getSExtValue();
}]>;

// Turn an immediate into an immZExt16 if the value can be expressed
// as a zero-extended 16-bit immediate
// instructions like ORi use zero-extended 16-bit immediates
def immZExt16  : PatLeaf<(imm), [{
  if (N->getValueType(0) == MVT::i32)
    return (uint32_t)N->getZExtValue() == (unsigned short)N->getZExtValue();
  else    
    return (uint64_t)N->getZExtValue() == (unsigned short)N->getZExtValue();
}], LO16>;

// shamt field must fit in 5 bits.
def immZExt5 : PatLeaf<(imm), [{
  return N->getZExtValue() == ((N->getZExtValue()) & 0x1f) ;
}]>;

// Node immediate fits in upper 16 bits and lower 16 bits are 0
def immUpperHalfword  : PatLeaf<(imm), [{
  if (N->getValueType(0) == MVT::i32)
    return ((uint32_t)N->getZExtValue() & (uint32_t)0xFFFF0000U) == (uint32_t)N->getZExtValue();
  else
    return ((uint64_t)N->getZExtValue() & (uint64_t)0xFFFF0000U) == (uint64_t)N->getZExtValue();
}], HI16>;

// Define a complex pattern to match subdags that need C++ select code.
// This pattern should call SelectAddr() in our backend, which takes 2 operands
// and returns an iPTR.  The only root of a subdag we want matched by this function
// is the FrameIndex SDNode.  The last argument lets you specify properties about
// the SDNodes you want to match, or in later versions of LLVM whether you want
// to pass the node itself, its parent, or something else to the C++ function.
def addr : ComplexPattern<iPTR, 2, "SelectAddr", [frameindex], []>;

//===----------------------------------------------------------------------===//
// One-off instructions without enough in common with others to make a class
//===----------------------------------------------------------------------===//

//FIXME We should have a NAND instruction; it's the cheapest one to do!
let isCommutable = 1, isAsCheapAsAMove = 1 in {
def NOR : RigelInst<
      (outs CPURegs:$dst),// OUTPUT
      (ins CPURegs:$a, CPURegs:$b),// INPUT
      !strconcat("nor", " $dst, $a, $b"),// ASM TO PRINT
      [(set CPURegs:$dst, (not (or CPURegs:$a, CPURegs:$b)))], //PATTERN TO MATCH
      IIAlu>; // EXEC UNIT TYPE
}

//===----------------------------------------------------------------------===//
// Reusable instruction classes
//===----------------------------------------------------------------------===//

// sext_inreg
class IntExtInReg<bits<5> sa, string instr_asm, SDNode OpNode, ValueType vt,
               RegisterClass RC>:
  FR<0x1f, 0x20, (outs RC:$dst), (ins RC:$src),
     !strconcat(instr_asm, " $dst, $src"),
     [(set RC:$dst, (OpNode RC:$src, vt))], NoItinerary> {
    let rs = 0;
    let shamt = sa;
    let Predicates = [];
}

// Arithmetic 3-register
let isCommutable = 1 in 
class ArithR<bits<6> op, bits<6> func, string instr_asm, SDNode OpNode,
             InstrItinClass itin>: 
  FR< op, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], itin>;

let isCommutable = 1 in 
class FArithR<bits<6> op, bits<6> func, string instr_asm, SDNode OpNode,
             InstrItinClass itin>: 
  FR< op, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], itin>;

let isCommutable = 1 in 
class ArithRUnsigned<bits<6> op, bits<6> func, string instr_asm, SDNode OpNode,
             InstrItinClass itin>: 
  FR< op, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], itin>;

let isCommutable = 0 in 
class ArithRUnsignedRev<bits<6> op, bits<6> func, string instr_asm, SDNode OpNode,
             InstrItinClass itin>: 
  FR< op, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $c, $b"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], itin>;

let isCommutable = 1 in 
class ArithOverflowR<bits<6> op, bits<6> func, string instr_asm>: 
  FR< op, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [], IIAlu>;

let isCommutable = 0 in 
class ArithOverflowRRev<bits<6> op, bits<6> func, string instr_asm>: 
  FR< op, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $c, $b"), 
      [], IIAlu>;

let isCommutable = 0 in
class ArithI<bits<6> op, string instr_asm, SDNode OpNode, 
             Operand Od, PatLeaf imm_type> : 
  FI< op, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, Od:$c), 
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, imm_type:$c))], IIAlu>;

// Logical 3-register
let isCommutable = 1 in {
class LogicR<bits<6> func, string instr_asm, SDNode OpNode>:
  FR< 0x00, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], IIAlu>;
}

class LogicRRev<bits<6> func, string instr_asm, SDNode OpNode>:
  FR< 0x00, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$b, CPURegs:$c), 
      !strconcat(instr_asm, " $dst, $c, $b"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], IIAlu>;

class LogicI<bits<6> op, string instr_asm, SDNode OpNode>:
  FI< op,
      (outs CPURegs:$dst),
      (ins CPURegs:$b, uimm16:$c),
      !strconcat(instr_asm, " $dst, $b, $c"),
      [(set CPURegs:$dst, (OpNode CPURegs:$b, immZExt16:$c))], IIAlu>;

// Shifts
let rt = 0 in
class LogicR_shift_imm<bits<6> func, string instr_asm, SDNode OpNode>:
  FR< 0x00, 
      func, 
      (outs CPURegs:$dst),
      (ins CPURegs:$b, shamt:$c),
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, immZExt5:$c))], IIAlu>;

class LogicR_shift_reg<bits<6> func, string instr_asm, SDNode OpNode>:
  FR< 0x00, 
      func, 
      (outs CPURegs:$dst),
      (ins CPURegs:$b, CPURegs:$c),
      !strconcat(instr_asm, " $dst, $b, $c"), 
      [(set CPURegs:$dst, (OpNode CPURegs:$b, CPURegs:$c))], IIAlu>;

// Load Upper Imediate
class LoadUpper<bits<6> op, string instr_asm>:
  FI< op,
      (outs CPURegs:$dst),
      (ins uimm16:$imm),
      !strconcat(instr_asm, " $dst, $imm"),
      [], IIAlu>;

// Memory Load/Store 
let canFoldAsLoad = 1 in
class LoadM<bits<6> op, string instr_asm, PatFrag OpNode, RegisterClass RC>:
  FI< op,
      (outs RC:$dst),
      (ins mem:$addr),
      !strconcat(instr_asm, " $dst, $addr"),
      [(set RC:$dst, (OpNode addr:$addr))], IILoad>;

//let isStore = 1 in
class StoreM<bits<6> op, string instr_asm, PatFrag OpNode, RegisterClass RC>:
  FI< op,
      (outs),
      (ins RC:$dst, mem:$addr),
      !strconcat(instr_asm, " $dst, $addr"),
      [(OpNode RC:$dst, addr:$addr)], IIStore>;

// Conditional Branch
let isBranch = 1, isTerminator=1 in {
let isCommutable = 1 in { //beq, bne are commutable.
class CBranch<bits<6> op, string instr_asm, PatFrag cond_op>:
  FI< op,
      (outs),
      (ins CPURegs:$a, CPURegs:$b, brtarget:$offset),
      !strconcat(instr_asm, " $a, $b, $offset"),
      [(brcond (cond_op CPURegs:$a, CPURegs:$b), bb:$offset)],
      IIBranch>;
}

class CBranchZero<bits<6> op, string instr_asm, PatFrag cond_op>:
  FI< op,
      (outs),
      (ins CPURegs:$src, brtarget:$offset),
      !strconcat(instr_asm, " $src, $offset"),
      [(brcond (cond_op CPURegs:$src, 0), bb:$offset)],
      IIBranch>;
}      

// Comparison operators
class Compare_RRev<bits<6> op, string instr_asm,
      PatFrag cond_op>:
  FR< op,
      0x0,
      (outs CPURegs:$dst),
      (ins CPURegs:$b, CPURegs:$c),
      !strconcat(instr_asm, " $dst, $c, $b"),
      [(set CPURegs:$dst, (cond_op CPURegs:$b, CPURegs:$c))],
      IIAlu>;

class Compare_R<bits<6> op, string instr_asm,
      PatFrag cond_op>:
  FR< op,
      0x0,
      (outs CPURegs:$dst),
      (ins CPURegs:$b, CPURegs:$c),
      !strconcat(instr_asm, " $dst, $b, $c"),
      [(set CPURegs:$dst, (cond_op CPURegs:$b, CPURegs:$c))],
      IIAlu>;
class Compare_FPRev<bits<6> op, string instr_asm,
      PatFrag cond_op>:
  FR< op,
      0x0,
      (outs CPURegs:$dst),
      (ins FPRegs:$b, FPRegs:$c),
      !strconcat(instr_asm, " $dst, $c, $b"),
      [(set CPURegs:$dst, (cond_op FPRegs:$b, FPRegs:$c))],
      IIAlu>;


class Compare_FP<bits<6> op, string instr_asm,
      PatFrag cond_op>:
  FR< op,
      0x0,
      (outs CPURegs:$dst),
      (ins FPRegs:$b, FPRegs:$c),
      !strconcat(instr_asm, " $dst, $b, $c"),
      [(set CPURegs:$dst, (cond_op FPRegs:$b, FPRegs:$c))],
      IIAlu>;

class SetCC_I<bits<6> op, string instr_asm, PatFrag cond_op,
      Operand Od, PatLeaf imm_type>:
  FI< op,
      (outs CPURegs:$dst),
      (ins CPURegs:$b, Od:$c),
      !strconcat(instr_asm, " $dst, $b, $c"),
      [(set CPURegs:$dst, (cond_op CPURegs:$b, imm_type:$c))],
      IIAlu>;

let isBranch=1, isTerminator=1, isBarrier=1 in
class JumpFJ<bits<6> op, string instr_asm>:
  FJ< op,
      (outs),
      (ins brtarget:$target),
      !strconcat(instr_asm, " $target"),
      [(br bb:$target)], IIBranch>;

let isBranch=1, isTerminator=1, isBarrier=1, rd=0, isIndirectBranch = 1 in
class JumpFR<bits<6> op, bits<6> func, string instr_asm>:
  FR< op,
      func,
      (outs),
      (ins CPURegs:$target),
      !strconcat(instr_asm, " $target"),
      [(brind CPURegs:$target)], IIBranch>;

// Jump and Link (Call)
//All calls clobber caller-saved registers.
//FIXME Do we need to define any Uses for call instrs?
//Mips Uses GP.
//TODO Can we reclaim K0/K1/AT?
let isCall=1, Defs = [ AT, K0, K1, V0, V1, GP, A0, A1, A2, A3, A4, A5, A6, A7,
                       T0, T1, T2, T3, T4, T5]
              in {

  class JumpLink<bits<6> op, string instr_asm>: 
    FJ< op,
        (outs),
        (ins calltarget:$target, variable_ops),
        !strconcat(instr_asm, "\t$target"),
       [(RigelJmpLink imm:$target)], IIBranch>;


  //let rd=31 in
  class JumpLinkReg<bits<6> func, string instr_asm>:
    FR< 0x00,
        func,
        (outs),
        (ins CPURegs:$rs, variable_ops),
        !strconcat(instr_asm, "\t$rs"),
        [(RigelJmpLink CPURegs:$rs)], IIBranch>;
}

// Mul, Div 
class MulDiv<bits<6> func, string instr_asm, InstrItinClass itin>: 
  FR< 0x00, 
      func, 
      (outs),
      (ins CPURegs:$a, CPURegs:$b), 
      !strconcat(instr_asm, " $a, $b"), 
      [], itin>;

class FMulDiv3Op<bits<6> func, string instr_asm, InstrItinClass itin>: 
  FR< 0x00, 
      func, 
      (outs FPRegs:$dest),
      (ins FPRegs:$a, FPRegs:$b), 
      !strconcat(instr_asm, " $dest, $a, $b"), 
      [], itin>;
			
class FMulDiv3OpRev<bits<6> func, string instr_asm, InstrItinClass itin>: 
  FR< 0x00, 
      func, 
      (outs FPRegs:$dest),
      (ins FPRegs:$a, FPRegs:$b), 
      !strconcat(instr_asm, " $dest, $b, $a"), 
      [], itin>;
			
class FMulDiv2Op<bits<6> func, string instr_asm, InstrItinClass itin>: 
  FR< 0x00, 
      func, 
      (outs FPRegs:$dest),
      (ins FPRegs:$a), 
      !strconcat(instr_asm, " $dest, $a"), 
      [], itin>;
			
class F2IClass<bits<6> func, string instr_asm, InstrItinClass itin>: 
  FR< 0x00, 
      func, 
      (outs CPURegs:$dest),
      (ins FPRegs:$a), 
      !strconcat(instr_asm, " $dest, $a"), 
      [(set CPURegs:$dest, (RigelFTOI FPRegs:$a))], itin>;
			
class I2FClass<bits<6> func, string instr_asm, InstrItinClass itin>: 
  FR< 0x00, 
      func, 
      (outs FPRegs:$dest),
      (ins CPURegs:$a), 
      !strconcat(instr_asm, " $dest, $a"), 
      [(set FPRegs:$dest, (RigelITOF CPURegs:$a))], itin>;
			
// Count Leading Ones/Zeros in Word
class CountLeading<bits<6> func, string instr_asm>:
  FR< 0x1c, 
      func, 
      (outs CPURegs:$dst), 
      (ins CPURegs:$src), 
      !strconcat(instr_asm, " $dst, $src"), 
      [], IIAlu>;

// AddrToReg Pseudo Op
class AddrToRegClass<bits<6> op, string instr_asm>:
  FI< op,
      (outs CPURegs:$dst),
      (ins mem:$addr),
      !strconcat(instr_asm, " $dst, $addr"),
      [], IILoad>;

// RegToAddrReg Pseudo Op
class RegToAddrReg<bits<6> op, string instr_asm>:
	FI< op,
			(outs mem:$addr),
			(ins CPURegs:$src),
			!strconcat(instr_asm, ""), //???? FIXME 
			[], IIAlu>;

def LEA			: AddrToRegClass<0x00, "addi">;

//===----------------------------------------------------------------------===//
// Pseudo instructions
//===----------------------------------------------------------------------===//

// As stack alignment is always done with ORi, we need a 16-bit immediate
let Defs = [SP], Uses = [SP] in {
def ADJCALLSTACKDOWN : RigelPseudo<(outs), (ins uimm16:$amt),
                              "!ADJCALLSTACKDOWN $amt",
                              [(callseq_start timm:$amt)]>;
def ADJCALLSTACKUP   : RigelPseudo<(outs), (ins uimm16:$amt1, uimm16:$amt2),
                              "!ADJCALLSTACKUP $amt1",
                              [(callseq_end timm:$amt1, timm:$amt2)]>;
}

// Some assembly macros need to avoid pseudoinstructions and assembler
// automatic reodering.
def MACRO     : RigelPseudo<(outs), (ins), ".set\tmacro",     []>;
def REORDER   : RigelPseudo<(outs), (ins), ".set\treorder",   []>;
def NOMACRO   : RigelPseudo<(outs), (ins), ".set\tnomacro",   []>;
def NOREORDER : RigelPseudo<(outs), (ins), ".set\tnoreorder", []>;

// The Rigel ISA doesn't have any instruction close to the SELECT_CC 
// operation. The solution is to create a Rigel pseudo SELECT_CC instruction
// (RigelSelectCC), use LowerSELECT_CC to generate this instruction and finally 
// replace it for real supported nodes into EmitInstrWithCustomInserter
let usesCustomInserter = 1 in {
  class PseudoSelCC<RegisterClass RC, string asmstr>:
    RigelPseudo<(outs RC:$dst), (ins CPURegs:$CmpRes, RC:$T, RC:$F), asmstr,
    [(set RC:$dst, (RigelSelectCC CPURegs:$CmpRes, RC:$T, RC:$F))]>;
}

def Select_CC_to_i : PseudoSelCC<CPURegs, "# RigelSelect_CC_i32">;
def Select_CC_to_f : PseudoSelCC<FPRegs, "# RigelSelect_CC_f32">;

let usesCustomInserter = 1 in {
  class PseudoFPSelCC<RegisterClass RC, string asmstr>:
    RigelPseudo<(outs RC:$dst), 
      (ins CPURegs:$CmpRes, RC:$T, RC:$F, condcode:$cc), asmstr,
    [(set RC:$dst, (RigelFPSelectCC CPURegs:$CmpRes, RC:$T, RC:$F, imm:$cc))]>;
}

def Select_FCC_to_i : PseudoFPSelCC<CPURegs, "# RigelSelect_FCC_i32">;
def Select_FCC_to_f : PseudoFPSelCC<FPRegs, "# RigelSelect_FCC_f32">;

//===----------------------------------------------------------------------===//
// Atomics
//===----------------------------------------------------------------------===//

// let usesCustomInserter = 1 in {
//     // def ATOMIC_LOAD_ADD_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_ADD_I8",
//     //   [(set CPURegs:$dst, (atomic_load_add_8 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_SUB_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_SUB_I8",
//     //   [(set CPURegs:$dst, (atomic_load_sub_8 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_AND_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_AND_I8",
//     //   [(set CPURegs:$dst, (atomic_load_and_8 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_OR_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_OR_I8",
//     //   [(set CPURegs:$dst, (atomic_load_or_8 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_XOR_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_XOR_I8",
//     //   [(set CPURegs:$dst, (atomic_load_xor_8 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_NAND_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_NAND_I8",
//     //   [(set CPURegs:$dst, (atomic_load_nand_8 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_ADD_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_ADD_I16",
//     //   [(set CPURegs:$dst, (atomic_load_add_16 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_SUB_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_SUB_I16",
//     //   [(set CPURegs:$dst, (atomic_load_sub_16 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_AND_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_AND_I16",
//     //   [(set CPURegs:$dst, (atomic_load_and_16 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_OR_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_OR_I16",
//     //   [(set CPURegs:$dst, (atomic_load_or_16 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_XOR_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_XOR_I16",
//     //   [(set CPURegs:$dst, (atomic_load_xor_16 CPURegs:$ptr, CPURegs:$incr))]>;
//     // def ATOMIC_LOAD_NAND_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_NAND_I16",
//     //   [(set CPURegs:$dst, (atomic_load_nand_16 CPURegs:$ptr, CPURegs:$incr))]>;
//     def ATOMIC_LOAD_ADD_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_ADD_I32",
//       [(set CPURegs:$dst, (atomic_load_add_32 CPURegs:$ptr, CPURegs:$incr))]>;
//     def ATOMIC_LOAD_SUB_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_SUB_I32",
//       [(set CPURegs:$dst, (atomic_load_sub_32 CPURegs:$ptr, CPURegs:$incr))]>;
//     def ATOMIC_LOAD_AND_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_AND_I32",
//       [(set CPURegs:$dst, (atomic_load_and_32 CPURegs:$ptr, CPURegs:$incr))]>;
//     def ATOMIC_LOAD_OR_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_OR_I32",
//       [(set CPURegs:$dst, (atomic_load_or_32 CPURegs:$ptr, CPURegs:$incr))]>;
//     def ATOMIC_LOAD_XOR_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_XOR_I32",
//       [(set CPURegs:$dst, (atomic_load_xor_32 CPURegs:$ptr, CPURegs:$incr))]>;
//     def ATOMIC_LOAD_NAND_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$incr), "# RigelATOMIC_LOAD_NAND_I32",
//       [(set CPURegs:$dst, (atomic_load_nand_32 CPURegs:$ptr, CPURegs:$incr))]>;

//     // def ATOMIC_SWAP_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$new), "# RigelATOMIC_SWAP_I8",
//     //   [(set CPURegs:$dst, (atomic_swap_8 CPURegs:$ptr, CPURegs:$new))]>;
//     // def ATOMIC_SWAP_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$new), "# RigelATOMIC_SWAP_I16",
//     //   [(set CPURegs:$dst, (atomic_swap_16 CPURegs:$ptr, CPURegs:$new))]>;
//     def ATOMIC_SWAP_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$new), "# RigelATOMIC_SWAP_I32",
//       [(set CPURegs:$dst, (atomic_swap_32 CPURegs:$ptr, CPURegs:$new))]>;

//     // def ATOMIC_CMP_SWAP_I8 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$old, CPURegs:$new), "# RigelATOMIC_CMP_SWAP_I8",
//     //   [(set CPURegs:$dst, (atomic_cmp_swap_8 CPURegs:$ptr, CPURegs:$old, CPURegs:$new))]>;
//     // def ATOMIC_CMP_SWAP_I16 : RigelPseudo<
//     //   (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$old, CPURegs:$new), "# RigelATOMIC_CMP_SWAP_I16",
//     //   [(set CPURegs:$dst, (atomic_cmp_swap_16 CPURegs:$ptr, CPURegs:$old, CPURegs:$new))]>;
//     def ATOMIC_CMP_SWAP_I32 : RigelPseudo<
//       (outs CPURegs:$dst), (ins CPURegs:$ptr, CPURegs:$old, CPURegs:$new), "# RigelATOMIC_CMP_SWAP_I32",
//       [(set CPURegs:$dst, (atomic_cmp_swap_32 CPURegs:$ptr, CPURegs:$old, CPURegs:$new))]>;
// }

let Constraints = "$swap = $dest" in {
def ATOMIC_CMP_SWAP_I32 : FR< 0x00, //FIXME RigelInstrFormats.td is busted anyway, so this doesn't matter for now.
                              0x00, //FIXME Ditto.
                              (outs CPURegs:$dest),
                              (ins CPURegs:$ptr, CPURegs:$compare, CPURegs:$swap),
                              "atom.cas\t$swap, $compare, $ptr",
                              [(set CPURegs:$dest, (atomic_cmp_swap_32 CPURegs:$ptr, CPURegs:$compare, CPURegs:$swap))],
                              IIStore>; 

def ATOMIC_SWAP_I32 : FR< 0x00,
                          0x00,
                          (outs CPURegs:$dest),
                          (ins CPURegs:$swap, CPURegs:$ptr),
                          "atom.xchg\t$swap, $ptr, 0",
                          [(set CPURegs:$dest, (atomic_swap_32 CPURegs:$ptr, CPURegs:$swap))],
                          IIStore>; 

} //Constraints = "$swap = $dest"
//FIXME Patterns for ptr+offset for atom.xchg (it has a simm16 field; use it!)

//===----------------------------------------------------------------------===//
// RigelI Instructions
//===----------------------------------------------------------------------===//

def MUL     : ArithR<0x1c, 0x02, "mul", mul, IIImul>;

let isAsCheapAsAMove = 1 in {
// Arithmetic

// This pattern is bizarre, we only use it to load 32-bit constants
// into registers (along with MVUI and the %hi() and %lo() macros).
// Somehow, the assembler knows how to manipulate the expansion of
// %hi() and %lo() to produce the correct value even when %lo() will
// be interpreted as negative when sign-extended.
def ADDiu   : ArithI<0x0A, "addi", add, uimm16, immZExt16>;

def ADDi    : ArithI<0x08, "addi", add, simm16, immSExt16>;
def SUBi    : ArithI<0x08, "subi",  sub, simm16, immSExt16>;
def ADDu    : ArithRUnsigned<0x00, 0x21, "add", add, IIAlu>;
def SUBu    : ArithRUnsignedRev<0x00, 0x23, "sub", sub, IIAlu>;
def ADD     : ArithOverflowR<0x00, 0x20, "add">;
def SUB     : ArithOverflowRRev<0x00, 0x22, "sub">; 

// Logical
def AND     : LogicR<0x24, "and", and>;
def OR      : LogicR<0x25, "or",  or>;
def XOR     : LogicR<0x26, "xor", xor>;
def SLL			: LogicRRev<0x27, "sll", shl>;
def SRL			: LogicRRev<0x28, "srl", srl>;
def SRA			: LogicRRev<0x29, "sra", sra>;
def ANDi    : LogicI<0x0c, "andi", and>;
def ORi     : LogicI<0x0d, "ori",  or>;
def XORi    : LogicI<0x0e, "xori",  xor>;

// Shifts 
def SLLI     : LogicR_shift_imm<0x00, "slli", shl>;
def SRLI     : LogicR_shift_imm<0x02, "srli", srl>;
def SRAI     : LogicR_shift_imm<0x03, "srai", sra>;

// Load Upper Immediate
def MVUi     : LoadUpper<0x0f, "mvui">;

// Sign extension
def SEXTB : IntExtInReg<0x10, "sext8", sext_inreg, i8, CPURegs>;
def SEXTS : IntExtInReg<0x18, "sext16", sext_inreg, i16, CPURegs>;

} //isAsCheapAsMove

// Load/Store
def LW     : LoadM<0x23, "ldw",  load, CPURegs>;
def LWFP   : LoadM<0x23, "ldw",  load, FPRegs>;

def SW     : StoreM<0x2b, "stw", store, CPURegs>;
def SWFP   : StoreM<0x2b, "stw", store, FPRegs>;

def BEQ    : CBranch<0x04, "beq", seteq>;
def BNE    : CBranch<0x05, "bne", setne>;

def BGE    : CBranchZero<0x01, "bge", setge>;
def BGT    : CBranchZero<0x07, "bgt", setgt>;
def BLE    : CBranchZero<0x07, "ble", setle>;
def BLT    : CBranchZero<0x01, "blt", setlt>;
def BE     : CBranchZero<0x01, "be",  seteq>;
def BNZ    : CBranchZero<0x01, "bnz", setne>;

let isAsCheapAsAMove = 1 in {
def CEQ			: Compare_RRev<0x00, "ceq", seteq>; 
def CLT 		: Compare_RRev<0x00, "clt", setlt>;
def CLTU		: Compare_RRev<0x00, "cltu", setult>;
def CLE			: Compare_RRev<0x00, "cle", setle>;
def CLEU		: Compare_RRev<0x00, "cleu", setule>; 
def CLTF		: Compare_FPRev<0x00, "cltf", setlt>; 
def CLTEF		: Compare_FPRev<0x00, "cltef", setle>;
def CEQF    : Compare_FPRev<0x00, "ceqf", seteq>;
}

def JMP     : JumpFJ<0x02, "lj">;
def JMPR    : JumpFR<0x00, 0x08, "jmpr">;

def JAL     : JumpLink<0x03, "ljl">;
def JALR    : JumpLinkReg<0x03, "jalr">; 

def FMUL    : FMulDiv3Op<0x1c, "fmul", IIFAlu>;
def FADD    : FMulDiv3Op<0x1c, "fadd", IIFAlu>;
def FSUB    : FMulDiv3OpRev<0x1c, "fsub", IIFAlu>;
def FRCP    : FMulDiv2Op<0x1c, "frcp", IIFAlu>;
def FABS    : FMulDiv2Op<0x1c, "fabs", IIFAlu>;
def FMRS    : FMulDiv2Op<0x1c, "fmrs", IIFAlu>;
def FRSQRT  : FMulDiv2Op<0x1c, "frsq", IIFAlu>;

def F2I     : F2IClass<0x1c, "f2i", IIImul>;
def I2F     : I2FClass<0x1c, "i2f", IIImul>;

def CTLZ : CountLeading<0x20, "clz">;

let addr=0 in
def NOP     : FJ<0, (outs), (ins), "nop", [], IIAlu>;

// Ret instruction - as rigel does not have "ret" a 
// jmpr $ra must be generated.
let isReturn=1, isTerminator=1,
    isBarrier=1, hasCtrlDep=1, rs=0, rt=0, shamt=0 in 
{
  def RET : FR <0x00, 0x02, (outs), (ins CPURegs:$target),
                "jmpr $target", [(RigelRet CPURegs:$target)], IIBranch>;
  def RET_NULL : FR <0x00, 0x02, (outs), (ins),
                "jmpr $$31", [(RigelRetNull)], IIBranch>;
}
//===----------------------------------------------------------------------===//
//  DAG Matching Patterns that use one or more of the above instr definitions
//===----------------------------------------------------------------------===//

//Count leading zeros
def : Pat <(ctlz CPURegs:$a), (CTLZ CPURegs:$a)>;

// Small immediates
def : Pat<(i32 immSExt16:$in), 
          (ADDi ZERO, imm:$in)>;
def : Pat<(i32 immZExt16:$in), 
          (ORi ZERO, imm:$in)>;
// Upper-halfword-only immediates
def : Pat<(i32 immUpperHalfword:$in),
          (MVUi (HI16 imm:$in)) >;
// Large immediates
// NOTE: This doesn't use ADDiu like most of the other ones because
// we can't rely on the assembler to manipulate %hi and %lo to deal
// with the sign-extendedness of the 'addi' instruction.
def imm32 : Pat<(i32 imm:$imm),
          (ORi (MVUi (HI16 imm:$imm)), (LO16 imm:$imm))>;

def : Pat<(RigelJmpLink (i32 tglobaladdr:$dst)),
          (JAL tglobaladdr:$dst)>;
def : Pat<(RigelJmpLink (i32 texternalsym:$dst)),
          (JAL texternalsym:$dst)>;

// hi/lo relocs
def : Pat<(RigelHi tglobaladdr:$in), (MVUi tglobaladdr:$in)>;
def : Pat<(RigelHi tblockaddress:$in), (MVUi tblockaddress:$in)>;
def : Pat<(RigelHi tjumptable:$in), (MVUi tjumptable:$in)>;
def : Pat<(RigelHi tconstpool:$in), (MVUi tconstpool:$in)>;
def : Pat<(RigelHi tglobaltlsaddr:$in), (MVUi tglobaltlsaddr:$in)>;

def : Pat<(RigelLo tglobaladdr:$in), (ADDiu ZERO, tglobaladdr:$in)>;
def : Pat<(RigelLo tblockaddress:$in), (ADDiu ZERO, tblockaddress:$in)>;
def : Pat<(RigelLo tjumptable:$in), (ADDiu ZERO, tjumptable:$in)>;
def : Pat<(RigelLo tconstpool:$in), (ADDiu ZERO, tconstpool:$in)>;
def : Pat<(RigelLo tglobaltlsaddr:$in), (ADDiu ZERO, tglobaltlsaddr:$in)>;

def : Pat<(add CPURegs:$hi, (RigelLo tglobaladdr:$lo)),
          (ADDiu CPURegs:$hi, tglobaladdr:$lo)>;
def : Pat<(add CPURegs:$hi, (RigelLo tblockaddress:$lo)),
          (ADDiu CPURegs:$hi, tblockaddress:$lo)>;
def : Pat<(add CPURegs:$hi, (RigelLo tjumptable:$lo)),
          (ADDiu CPURegs:$hi, tjumptable:$lo)>;
def : Pat<(add CPURegs:$hi, (RigelLo tconstpool:$lo)),
          (ADDiu CPURegs:$hi, tconstpool:$lo)>;
def : Pat<(add CPURegs:$hi, (RigelLo tglobaltlsaddr:$lo)),
          (ADDiu CPURegs:$hi, tglobaltlsaddr:$lo)>;

def : Pat<(add (RigelHi tglobaladdr:$in1), (RigelLo tglobaladdr:$in2)),
          (ADDiu (MVUi tglobaladdr:$in1), (tglobaladdr:$in2)) >;
def : Pat<(add (RigelHi tblockaddress:$in1), (RigelLo tblockaddress:$in2)),
          (ADDiu (MVUi tblockaddress:$in1), (tblockaddress:$in2)) >;
def : Pat<(add (RigelHi tjumptable:$in1), (RigelLo tjumptable:$in2)),
          (ADDiu (MVUi tjumptable:$in1), (tjumptable:$in2)) >;
def : Pat<(add (RigelHi tconstpool:$in1), (RigelLo tconstpool:$in2)),
          (ADDiu (MVUi tconstpool:$in1), (tconstpool:$in2)) >;
def : Pat<(add (RigelHi tglobaltlsaddr:$in1), (RigelLo tglobaltlsaddr:$in2)),
          (ADDiu (MVUi tglobaltlsaddr:$in1), (tglobaltlsaddr:$in2)) >;

// We synthesize a 'not' operation as 'nor $zero'.
// This could be made an assembler macro to allow people to write 'not' in asm.
def : Pat<(not CPURegs:$in),
          (NOR CPURegs:$in, ZERO)>;

// sint <-> fp instructions
// Our F2I and I2F assume the int value is signed.
def : Pat<(i32 (fp_to_sint FPRegs:$src)), 
									(F2I FPRegs:$src)>;
def : Pat<(f32 (sint_to_fp CPURegs:$src)), 
									(I2F CPURegs:$src)>;


def : Pat<(fabs FPRegs:$src), 
                  (FABS FPRegs:$src)>;
//FIXME is this precise enough?  X86 backend suggests they usually need
//refinement to meet precision requirements.
def : Pat<(fsqrt FPRegs:$src),
                  (FRCP (FRSQRT FPRegs:$src))>;

def : Pat<(f32 (fmul FPRegs:$a, FPRegs:$b)), 
	(FMUL FPRegs:$a, FPRegs:$b)>;
def : Pat<(f32 (fadd FPRegs:$a, FPRegs:$b)), 
	(FADD FPRegs:$a, FPRegs:$b)>;
def : Pat<(f32 (fsub FPRegs:$a, FPRegs:$b)), 
	(FSUB FPRegs:$a, FPRegs:$b)>;
//FIXME is this precise enough?  We may need to only turn this on under
//-ffast-math, or maybe LLVM already knows how to do this and this pattern
//is unnecessary
def : Pat<(fdiv FPRegs:$a, FPRegs:$b), 
	(FMUL FPRegs:$a, (FRCP FPRegs:$b))>;

//===---------===//
//  Branches
//===---------===//

def : Pat<(seteq FPRegs:$a, FPRegs:$b),
           (CEQF FPRegs:$a, FPRegs:$b)>;
def : Pat<(setne FPRegs:$a, FPRegs:$b),
           (XORi (CEQF FPRegs:$a, FPRegs:$b), 1)>;

// generic brcond pattern
 def : Pat<(brcond CPURegs:$cond, bb:$dst),
          (BNE CPURegs:$cond, ZERO, bb:$dst)>;

// the pattern (brcond (CondCode op1, op2), bb$dst) means "branch if the relationship
// between op1 and op2 is described by CondCode"; below we translate each CondCode into
// Rigel instructions so that we branch in the appropriate situations

// integers have to define SETEQ,SETNE,SETLT,SETLE,SETGT, 
// SETGE,SETULT,SETULE,SETUGT, and SETUGE patterns

// direct comparisons
def : Pat<(brcond (seteq CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BEQ CPURegs:$lhs, CPURegs:$rhs, bb:$dst)>;
def : Pat<(brcond (setne CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNE CPURegs:$lhs, CPURegs:$rhs, bb:$dst)>;

// indirect comparisons in Rigel are zero-based, and subtraction side-effects,
// so we use comparison operators instead
// Rigel comparison instructions return 1 when the comparison holds, 0 otherwise
def : Pat<(brcond (setlt CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLT CPURegs:$lhs, CPURegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setle CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLE CPURegs:$lhs, CPURegs:$rhs), bb:$dst)>;

def : Pat<(brcond (setgt CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLT CPURegs:$rhs, CPURegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setge CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLE CPURegs:$rhs, CPURegs:$lhs), bb:$dst)>;

// unsigned variants
def : Pat<(brcond (setult CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLTU CPURegs:$lhs, CPURegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setule CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLEU CPURegs:$lhs, CPURegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setugt CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLTU CPURegs:$rhs, CPURegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setuge CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNZ (CLEU CPURegs:$rhs, CPURegs:$lhs), bb:$dst)>;

def : Pat<(brcond (setune CPURegs:$lhs, CPURegs:$rhs), bb:$dst),
            (BNE CPURegs:$lhs, CPURegs:$rhs, bb:$dst)>;

// optimize direct comparisons to 0
def : Pat<(brcond (seteq CPURegs:$lhs, 0), bb:$dst),
            (BE CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setne CPURegs:$lhs, 0), bb:$dst),
            (BNZ CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setgt CPURegs:$lhs, 0), bb:$dst),
            (BGT CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setge CPURegs:$lhs, 0), bb:$dst),
            (BGE CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setle CPURegs:$lhs, 0), bb:$dst),
            (BLE CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setlt CPURegs:$lhs, 0), bb:$dst),
            (BLT CPURegs:$lhs, bb:$dst)>;

// NOTE: LLVM IR canonicalizes <= and >= to < and >, so we need explicit patterns
// to match IR generated this way that can be handled by our implicit-zero branching instrs.
def : Pat<(brcond (setgt CPURegs:$lhs, -1), bb:$dst),
            (BGE CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setlt CPURegs:$lhs, 1), bb:$dst),
            (BLE CPURegs:$lhs, bb:$dst)>;
// Probably not necessary because of the preference for < and >, but we'll put them in anyway.
def : Pat<(brcond (setge CPURegs:$lhs, 1), bb:$dst),
            (BGT CPURegs:$lhs, bb:$dst)>;
def : Pat<(brcond (setle CPURegs:$lhs, -1), bb:$dst),
            (BLT CPURegs:$lhs, bb:$dst)>;

// floats have to define all operations, even the ones with NaNs
// first, ordered floating-point: true only if condition holds
def : Pat<(brcond (setoeq FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CEQF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setone FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BE (CEQF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setogt FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTF FPRegs:$rhs, FPRegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setoge FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTEF FPRegs:$rhs, FPRegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setolt FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setole FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTEF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;

// unordered floating-point stuff: supposed to return true if the comparison holds
// *or* if any operand is a NaN, but we don't support NaNs yet, so these are materially
// the same as the above
// FIXME Extend ISA semantics to handle NaNs consistently, propagate those semantics here

def : Pat<(brcond (setueq FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CEQF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setune FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BE (CEQF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setugt FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTF FPRegs:$rhs, FPRegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setuge FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTEF FPRegs:$rhs, FPRegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setult FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setule FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTEF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;

def : Pat<(setueq FPRegs:$lhs, FPRegs:$rhs),
           (CEQF FPRegs:$lhs, FPRegs:$rhs)>; 
def : Pat<(setune FPRegs:$lhs, FPRegs:$rhs),
           (XORi (CEQF FPRegs:$lhs, FPRegs:$rhs), 1 )>;
def : Pat<(setule FPRegs:$lhs, FPRegs:$rhs),
            (CLTEF FPRegs:$lhs, FPRegs:$rhs)>;
def : Pat<(setult FPRegs:$lhs, FPRegs:$rhs),
            (CLTF FPRegs:$lhs, FPRegs:$rhs)>;
def : Pat<(setuge FPRegs:$lhs, FPRegs:$rhs),
            (CLTEF FPRegs:$rhs, FPRegs:$lhs)>;
def : Pat<(setugt FPRegs:$lhs, FPRegs:$rhs),
            (CLTF FPRegs:$rhs, FPRegs:$lhs)>;

// For FP compares without an ordering constraint, we use the unordered patterns
def : Pat<(brcond (seteq FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CEQF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setne FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BE (CEQF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setgt FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTF FPRegs:$rhs, FPRegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setge FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTEF FPRegs:$rhs, FPRegs:$lhs), bb:$dst)>;
def : Pat<(brcond (setlt FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;
def : Pat<(brcond (setle FPRegs:$lhs, FPRegs:$rhs), bb:$dst),
            (BNZ (CLTEF FPRegs:$lhs, FPRegs:$rhs), bb:$dst)>;


// See http://llvm.org/docs/LangRef.html.
// In FP condition codes of the form "setoXX", "o" means ordered, i.e. neither operand is a QNaN.
// In FP condition codes of the form "setuXX", "u" means unordered, i.e. either operand may be a QNaN.
// we don't support QNaN's yet, but if we do we need to make sure that the o and u CC nodes are handled
// properly.

//Set ordered and unordered.  2 FP values are ordered if neither is a NaN, and unordered if either is a NaN.
//We test for NaN by testing a==a and b==b (using the floating-point comparison, not integer, obviously).
def : Pat<(seto FPRegs:$lhs, FPRegs:$rhs),
            (AND (CEQF FPRegs:$lhs, FPRegs:$lhs), (CEQF FPRegs:$rhs, FPRegs:$rhs) )>;
def : Pat<(setuo FPRegs:$lhs, FPRegs:$rhs),
            (XORi (AND (CEQF FPRegs:$lhs, FPRegs:$lhs), (CEQF FPRegs:$rhs, FPRegs:$rhs)), 1 )>;

def : Pat<(setole FPRegs:$lhs, FPRegs:$rhs),
            (CLTEF FPRegs:$lhs, FPRegs:$rhs)>;
def : Pat<(setolt FPRegs:$lhs, FPRegs:$rhs),
            (CLTF FPRegs:$lhs, FPRegs:$rhs)>;
def : Pat<(setoge FPRegs:$lhs, FPRegs:$rhs),
            (CLTF FPRegs:$rhs, FPRegs:$lhs)>;
def : Pat<(setogt FPRegs:$lhs, FPRegs:$rhs),
            (CLTEF FPRegs:$rhs, FPRegs:$lhs)>;
def : Pat<(setoeq FPRegs:$lhs, FPRegs:$rhs),
            (CEQF FPRegs:$lhs, FPRegs:$rhs)>;
def : Pat<(setone FPRegs:$lhs, FPRegs:$rhs),
            (XORi (CEQF FPRegs:$lhs, FPRegs:$rhs), 1 )>;

// setcc 2 register operands
def : Pat<(setune CPURegs:$lhs, CPURegs:$rhs),
          (XORi (CEQ CPURegs:$lhs, CPURegs:$rhs), 1 )>;
def : Pat<(setueq CPURegs:$lhs, CPURegs:$rhs),
          (CEQ CPURegs:$lhs, CPURegs:$rhs)>;
def : Pat<(setuge CPURegs:$lhs, CPURegs:$rhs),
          (CLEU CPURegs:$rhs, CPURegs:$lhs )>;
def : Pat<(setugt CPURegs:$lhs, CPURegs:$rhs),
          (CLTU CPURegs:$rhs, CPURegs:$lhs)>;
def : Pat<(setule CPURegs:$lhs, CPURegs:$rhs),
          (CLEU CPURegs:$lhs, CPURegs:$rhs)>;
def : Pat<(setult CPURegs:$lhs, CPURegs:$rhs),
          (CLTU CPURegs:$lhs, CPURegs:$rhs)>;

def : Pat<(setne CPURegs:$a, CPURegs:$b),
          (XORi (CEQ CPURegs:$a, CPURegs:$b), 1 )>;
def : Pat<(seteq CPURegs:$a, CPURegs:$b),
          (CEQ CPURegs:$a, CPURegs:$b)>;
def : Pat<(setge CPURegs:$a, CPURegs:$b),
          (CLE CPURegs:$b, CPURegs:$a)>;
def : Pat<(setgt CPURegs:$lhs, CPURegs:$rhs),
          (CLT CPURegs:$rhs, CPURegs:$lhs)>;
def : Pat<(setle CPURegs:$a, CPURegs:$b),
          (CLE CPURegs:$a, CPURegs:$b)>;
def : Pat<(setlt CPURegs:$a, CPURegs:$b),
          (CLT CPURegs:$a, CPURegs:$b)>;
          
// setcc reg/imm operands
def : Pat<(setge CPURegs:$lhs, immSExt16:$rhs),
          (CLE (i32 immSExt16:$rhs), CPURegs:$lhs )>;
def : Pat<(setuge CPURegs:$lhs, immZExt16:$rhs),
          (CLEU (i32 immZExt16:$rhs), CPURegs:$lhs )>;
