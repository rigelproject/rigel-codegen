//===- Rigel.td - Describe the Rigel Target Machine ---------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
// This defines the supported Rigel processors and their features.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Register File, Calling Convention, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "RigelRegisterInfo.td"
include "RigelSchedule.td"
include "RigelInstrInfo.td"
include "RigelCallingConv.td"

def RigelInstrInfo : InstrInfo;

def FeatureByteLoadStore : SubtargetFeature<"byteldst", "HasByteLdSt",
                                            "true", "Has byte-wide loads and stores">;

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, RigelGenericItineraries, Features>;

//Support two subtargets, rfunctional and rstructural, to show how multiple sets of
//ISA features can coexist.
def : Proc<"rfunctional", [FeatureByteLoadStore]>;
def : Proc<"rstructural", []>;

def Rigel : Target {
  let InstructionSet = RigelInstrInfo;
}
