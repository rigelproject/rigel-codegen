//===- RigelSchedule.td - Rigel Scheduling Definitions ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Functional units across Rigel chips sets. Based on GCC/Rigel backend files.
//===----------------------------------------------------------------------===//
def ALU1     : FuncUnit;
def ALU2     : FuncUnit;
def FPU1     : FuncUnit;
def LSU			 : FuncUnit;
def IMULDIV  : FuncUnit;
//FIXME Add more FUs and itineraries to account for the asymmetric pipes in
//the structural core

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for Rigel 
//===----------------------------------------------------------------------===//
def IIAlu              : InstrItinClass;
def IILoad             : InstrItinClass;
def IIStore            : InstrItinClass;
def IIBranch           : InstrItinClass;
def IIImul             : InstrItinClass;
def IIFAlu	       : InstrItinClass;
def IIPseudo           : InstrItinClass;

//===----------------------------------------------------------------------===//
// Rigel Generic instruction itineraries.
//===----------------------------------------------------------------------===//
def RigelGenericItineraries : ProcessorItineraries<[ALU1,ALU2,LSU,IMULDIV,FPU1], [
  InstrItinData<IIAlu              , [InstrStage<1,  [ALU1, ALU2]>]>,
  InstrItinData<IILoad             , [InstrStage<1,  [LSU]>]>,
  InstrItinData<IIStore            , [InstrStage<1,  [LSU]>]>,
  InstrItinData<IIBranch           , [InstrStage<2,  [ALU1, ALU2]>]>,
  InstrItinData<IIImul             , [InstrStage<2,  [IMULDIV]>]>,
  InstrItinData<IIFAlu             , [InstrStage<4,  [FPU1]>]>
]>;
