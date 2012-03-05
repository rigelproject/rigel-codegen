//===-- RigelAsmPrinter.cpp - Rigel LLVM assembly writer --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format Rigel assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "rigel-asm-printer"

#include "Rigel.h"
#include "RigelSubtarget.h"
#include "RigelInstrInfo.h"
#include "RigelTargetMachine.h"
#include "RigelMachineFunction.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Target/Mangler.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
  class RigelAsmPrinter : public AsmPrinter {
    const RigelSubtarget *Subtarget;
  public:
    explicit RigelAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
       : AsmPrinter(TM, Streamer) {
      Subtarget = &TM.getSubtarget<RigelSubtarget>();
    }

    virtual const char *getPassName() const {
      return "Rigel Assembly Printer";
    }

    bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, 
                         unsigned AsmVariant, const char *ExtraCode, raw_ostream &O);
    bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo, 
                         unsigned AsmVariant, const char *ExtraCode, raw_ostream &O);
    void printOperand(const MachineInstr *MI, int opNum, raw_ostream &O);
    void printUnsignedImm(const MachineInstr *MI, int opNum, raw_ostream &O);
    void printMemOperand(const MachineInstr *MI, int opNum, raw_ostream &O, 
                         const char *Modifier = 0);
    //void printModuleLevelGV(const GlobalVariable* GVar);
    void printSavedRegsBitmask(raw_ostream &O);
    void printHex32(unsigned int Value, raw_ostream &O);

    //const char *emitCurrentABIString(void);
    void emitFrameDirective();

    void printInstruction(const MachineInstr *MI, raw_ostream &O);  // autogenerated.
    // targets are required to implement EmitInstruction
    void EmitInstruction(const MachineInstr *MI) {
      SmallString<128> Str;
      raw_svector_ostream OS(Str);
      printInstruction(MI, OS);
      OutStreamer.EmitRawText(OS.str());
    }

    virtual void EmitFunctionBodyStart();
    virtual void EmitFunctionBodyEnd();
    static const char *getRegisterName(unsigned RegNo);

    virtual void EmitFunctionEntryLabel();
  };
} // end of anonymous namespace

#include "RigelGenAsmWriter.inc"

//===----------------------------------------------------------------------===//
//
//  Rigel Asm Directives
//
//  -- Frame directive "frame Stackpointer, Stacksize, RARegister"
//  Describe the stack frame.
//
//  -- Mask directives "(f)mask  bitmask, offset" 
//  Tells the assembler which registers are saved and where.
//  bitmask - contain a little endian bitset indicating which registers are 
//            saved on function prologue (e.g. with a 0x80000000 mask, the 
//            assembler knows the register 31 (RA) is saved at prologue.
//  offset  - the position before stack pointer subtraction indicating where 
//            the first saved register on prologue is located. (e.g. with a
//
//  Consider the following function prologue:
//
//    .frame  $fp,48,$ra
//    .mask   0xc0000000,-8
//       addiu $sp, $sp, -48
//       sw $ra, 40($sp)
//       sw $fp, 36($sp)
//
//    With a 0xc0000000 mask, the assembler knows the register 31 (RA) and 
//    30 (FP) are saved at prologue. As the save order on prologue is from 
//    left to right, RA is saved first. A -8 offset means that after the 
//    stack pointer subtration, the first register in the mask (RA) will be
//    saved at address 48-8=40.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Mask directives
//===----------------------------------------------------------------------===//

// Create a bitmask with all callee saved registers for CPU or Floating Point 
// registers. For CPU registers consider RA, GP and FP for saving if necessary.
void RigelAsmPrinter::
printSavedRegsBitmask(raw_ostream &O)
{
  const TargetRegisterInfo &RI = *TM.getRegisterInfo();
  const RigelFunctionInfo *RigelFI = MF->getInfo<RigelFunctionInfo>();
             
  // CPU and FPU Saved Registers Bitmasks
  unsigned int CPUBitmask = 0;
  unsigned int FPUBitmask = 0;

  // Set the CPU and FPU Bitmasks
  const MachineFrameInfo *MFI = MF->getFrameInfo();
  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    unsigned Reg = CSI[i].getReg();
    unsigned RegNum = RigelRegisterInfo::getRegisterNumbering(Reg);
    if (Rigel::CPURegsRegisterClass->contains(Reg))
      CPUBitmask |= (1 << RegNum);
    else
      FPUBitmask |= (1 << RegNum);
  }

  // Return Address and Frame registers must also be set in CPUBitmask.
  if (RI.hasFP(*MF)) 
    CPUBitmask |= (1 << RigelRegisterInfo::
                getRegisterNumbering(RI.getFrameRegister(*MF)));
  
  if (MF->getFrameInfo()->hasCalls()) 
    CPUBitmask |= (1 << RigelRegisterInfo::
                getRegisterNumbering(RI.getRARegister()));

  // Print CPUBitmask
  O << "\t.mask \t"; printHex32(CPUBitmask, O); 
  O << ',' << RigelFI->getCPUTopSavedRegOff() << '\n';

  // Print FPUBitmask
  O << "\t.fmask\t"; printHex32(FPUBitmask, O); 
  O << "," << RigelFI->getFPUTopSavedRegOff() << '\n';
}

// Print a 32 bit hex number with all numbers.
void RigelAsmPrinter::
printHex32(unsigned int Value, raw_ostream &O) 
{
  O << "0x";
  for (int i = 7; i >= 0; i--) 
    O << utohexstr( (Value & (0xF << (i*4))) >> (i*4) );
}

//===----------------------------------------------------------------------===//
// Frame and Set directives
//===----------------------------------------------------------------------===//

void RigelAsmPrinter::EmitFunctionEntryLabel() {
  OutStreamer.EmitRawText("\t.ent\t" + Twine(CurrentFnSym->getName()));
  OutStreamer.EmitLabel(CurrentFnSym);
}

/// EmitFunctionBodyStart - Targets can override this to emit stuff before
/// the first basic block in the function.
void RigelAsmPrinter::EmitFunctionBodyStart() {  
  SmallString<128> Str;
  raw_svector_ostream OS(Str);
  printSavedRegsBitmask(OS);
  OutStreamer.EmitRawText(OS.str());
}

/// EmitFunctionBodyEnd - Targets can override this to emit stuff after
/// the last basic block in the function.
void RigelAsmPrinter::EmitFunctionBodyEnd() {
  OutStreamer.EmitRawText("\t.end\t" + Twine(CurrentFnSym->getName()));
}

// Print out an operand for an inline asm expression.
bool RigelAsmPrinter:: PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, 
                unsigned AsmVariant, const char *ExtraCode,
		raw_ostream &O) 
{
  // Does this asm operand have a single letter operand modifier?
  if (ExtraCode && ExtraCode[0]) 
    return true; // Unknown modifier.

  printOperand(MI, OpNo, O);
  return false;
}

//FIXME Is this still needed?
bool RigelAsmPrinter:: PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo, 
                unsigned AsmVariant, const char *ExtraCode,
		raw_ostream &O) 
{
  // Does this asm operand have a single letter operand modifier?
  if (ExtraCode && ExtraCode[0]) 
    return true; // Unknown modifier.

  printMemOperand(MI, OpNo, O);
  return false;
}

void RigelAsmPrinter:: printOperand(const MachineInstr *MI, int opNum,
				raw_ostream &O) 
{
  const MachineOperand &MO = MI->getOperand(opNum);
  bool isPIC = (TM.getRelocationModel() == Reloc::PIC_);
  bool isCodeLarge = (TM.getCodeModel() == CodeModel::Large);
 
  switch (MO.getType()) 
  {
    case MachineOperand::MO_Register:
        O << '$' << LowercaseString (getRegisterName(MO.getReg()));
      break;

    case MachineOperand::MO_Immediate:
      // We may need to add more here
      if ((MI->getOpcode() == Rigel::ORi)  || (MI->getOpcode() == Rigel::ANDi)
       || (MI->getOpcode() == Rigel::XORi) || (MI->getOpcode() == Rigel::MVUi)) {
        O << (unsigned short int)MO.getImm();
      } else {
        O << (short int)MO.getImm();
      }
      break;


    case MachineOperand::MO_MachineBasicBlock:
       O << *MO.getMBB()->getSymbol();
      return;


    case MachineOperand::MO_GlobalAddress:
      if (MI->getOpcode() == Rigel::MVUi) {
        O << "%hi(" << *Mang->getSymbol(MO.getGlobal()) << ")";
      } else {
        O << *Mang->getSymbol(MO.getGlobal());
      }
      break;

    case MachineOperand::MO_ExternalSymbol:
      O << *GetExternalSymbolSymbol(MO.getSymbolName());
      break;

    case MachineOperand::MO_JumpTableIndex:
			if (MI->getOpcode() == Rigel::MVUi)
				O << "%hi(";
      O << MAI->getPrivateGlobalPrefix() << "JTI" << getFunctionNumber()
        << '_' << MO.getIndex();
			if (MI->getOpcode() == Rigel::MVUi)
				O << ")";
      break;

    case MachineOperand::MO_ConstantPoolIndex:
      if(MI->getOpcode() == Rigel::MVUi)
        O << "%hi(";
      O << MAI->getPrivateGlobalPrefix() << "CPI"
        << getFunctionNumber() << "_" << MO.getIndex();
      if (MO.getOffset())
        O << "+" << MO.getOffset();
      if(MI->getOpcode() == Rigel::MVUi)
        O << ")";
      break;
  
    default:
      llvm_unreachable("<unknown operand type>"); 
  }
}

void RigelAsmPrinter:: printUnsignedImm(const MachineInstr *MI, int opNum,
			raw_ostream &O) 
{
  const MachineOperand &MO = MI->getOperand(opNum);
  if (MO.isImm())
    O << (unsigned short int)MO.getImm();
  else 
    printOperand(MI, opNum, O);
}

void RigelAsmPrinter::
printMemOperand(const MachineInstr *MI, int opNum, raw_ostream &O, 
		const char *Modifier) 
{
  if (MI->getOpcode() == Rigel::JALR) {
    printOperand(MI, opNum+1, O);
  }
  else {
    printOperand(MI, opNum+1, O);
    O << ", ";
    printOperand(MI, opNum, O);
  }
}

extern "C" void LLVMInitializeRigelAsmPrinter() {
  RegisterAsmPrinter<RigelAsmPrinter> X(TheRigelTarget);
  //RegisterAsmPrinter<RigelAsmPrinter> Y(TheRigelelTarget); // what does this mean?
}