//===-- RigelTargetMachine.h - Define TargetMachine for Rigel -00--*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Rigel specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef RIGELTARGETMACHINE_H
#define RIGELTARGETMACHINE_H

#include "RigelSubtarget.h"
#include "RigelInstrInfo.h"
#include "RigelISelLowering.h"
#include "RigelSelectionDAGInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetFrameInfo.h"

namespace llvm {
  class formatted_raw_ostream;

  class RigelTargetMachine : public LLVMTargetMachine {
    RigelSubtarget        Subtarget;
    const TargetData      DataLayout; // Calculates type size & alignment
    RigelInstrInfo        InstrInfo;
    TargetFrameInfo       FrameInfo;
    RigelTargetLowering   TLInfo;
    RigelSelectionDAGInfo TSInfo;
    InstrItineraryData    InstrItins;
  
  public:
    RigelTargetMachine(const Target &T, const std::string &TT,
                       const std::string &FS);

    virtual const RigelInstrInfo *getInstrInfo() const { return &InstrInfo; }
    virtual const TargetFrameInfo *getFrameInfo() const { return &FrameInfo; }
    virtual const RigelSubtarget *getSubtargetImpl() const { return &Subtarget; }
    virtual const TargetData *getTargetData() const { return &DataLayout; }
    virtual const RigelRegisterInfo *getRegisterInfo() const {
      return &InstrInfo.getRegisterInfo();
    }
    virtual const RigelTargetLowering *getTargetLowering() const { 
      return &TLInfo; 
    }
    virtual const RigelSelectionDAGInfo *getSelectionDAGInfo() const { 
      return &TSInfo; 
    }
    virtual const InstrItineraryData getInstrItineraryData() const {  return InstrItins; }
    static unsigned getModuleMatchQuality(const Module &M);
    // Pass Pipeline Configuration
    virtual bool addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
    virtual bool addPreEmitPass(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
		virtual bool addPreSched2(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
  };
} // End llvm namespace

#endif
