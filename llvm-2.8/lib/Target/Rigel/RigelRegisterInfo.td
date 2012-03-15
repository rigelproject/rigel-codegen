//===- RigelRegisterInfo.td - Rigel Register defs -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the RIGEL register file
//===----------------------------------------------------------------------===//

//FIXME This implies we have 64 registers
class RigelReg<string n> : Register<n> {
  field bits<6> Num;
  let Namespace = "Rigel";
}
class RigelGPRReg<bits<6> num, string n> : RigelReg<n> {
  let Num = num;
}

//===----------------------------------------------------------------------===//
//  Registers
//===----------------------------------------------------------------------===//

let Namespace = "Rigel" in {

  // General Purpose Registers
  def ZERO : RigelGPRReg< 0, "ZERO">, DwarfRegNum<[0]>;
  def AT : RigelGPRReg< 1, "1">, DwarfRegNum<[1]>;
  def V0 : RigelGPRReg< 2, "2">, DwarfRegNum<[2]>;
  def V1 : RigelGPRReg< 3, "3">, DwarfRegNum<[3]>;
  def A0 : RigelGPRReg< 4, "4">, DwarfRegNum<[4]>;
  def A1 : RigelGPRReg< 5, "5">, DwarfRegNum<[5]>;
  def A2 : RigelGPRReg< 6, "6">, DwarfRegNum<[6]>;
  def A3 : RigelGPRReg< 7, "7">, DwarfRegNum<[7]>;
  def A4 : RigelGPRReg< 8, "8">, DwarfRegNum<[8]>;
  def A5 : RigelGPRReg< 9, "9">, DwarfRegNum<[9]>;
  def A6 : RigelGPRReg< 10, "10">, DwarfRegNum<[10]>;
  def A7 : RigelGPRReg< 11, "11">, DwarfRegNum<[11]>;
  def T0 : RigelGPRReg< 12, "12">, DwarfRegNum<[12]>;
  def T1 : RigelGPRReg< 13, "13">, DwarfRegNum<[13]>;
  def T2 : RigelGPRReg< 14, "14">, DwarfRegNum<[14]>;
  def T3 : RigelGPRReg< 15, "15">, DwarfRegNum<[15]>;
  def T4 : RigelGPRReg< 16, "16">, DwarfRegNum<[16]>;
  def T5 : RigelGPRReg< 17, "17">, DwarfRegNum<[17]>;
  def S0 : RigelGPRReg< 18, "18">, DwarfRegNum<[18]>;
  def S1 : RigelGPRReg< 19, "19">, DwarfRegNum<[19]>;
  def S2 : RigelGPRReg< 20, "20">, DwarfRegNum<[20]>;
  def S3 : RigelGPRReg< 21, "21">, DwarfRegNum<[21]>;
  def S4 : RigelGPRReg< 22, "22">, DwarfRegNum<[22]>;
  def S5 : RigelGPRReg< 23, "23">, DwarfRegNum<[23]>;
  def S6 : RigelGPRReg< 24, "24">, DwarfRegNum<[24]>;
  def S7 : RigelGPRReg< 25, "25">, DwarfRegNum<[25]>;
  def K0 : RigelGPRReg< 26, "26">, DwarfRegNum<[26]>;
  def K1 : RigelGPRReg< 27, "27">, DwarfRegNum<[27]>;
  def GP : RigelGPRReg< 28, "GP">, DwarfRegNum<[28]>;
  def SP : RigelGPRReg< 29, "SP">, DwarfRegNum<[29]>;
  def FP : RigelGPRReg< 30, "FP">, DwarfRegNum<[30]>;
  def RA : RigelGPRReg< 31, "RA">, DwarfRegNum<[31]>;
}

// CPU Registers Class
def CPURegs : RegisterClass<"Rigel", [i32], 32, 
  // Allocate scratch registers first, then callee-saved registers
  [T0, T1, T2, T3, T4, T5, K0, K1, GP, AT, //FIXME GP should be reserved under PIC
   A0, A1, A2, A3, A4, A5, A6, A7, V0, V1,
   S0, S1, S2, S3, S4, S5, S6, S7, //Callee-saved
   ZERO, SP, FP, RA //Reserved
  ]>
{
  let MethodProtos = [{
    iterator allocation_order_end(const MachineFunction &MF) const;
  }];
  let MethodBodies = [{
    CPURegsClass::iterator
    CPURegsClass::allocation_order_end(const MachineFunction &MF) const {
      // The last 4 registers on the list above are reserved, can allocate through S7.
      return end()-4;
    }
  }];
}

//FIXME Should not need a separate RC for FP
def FPRegs : RegisterClass<"Rigel", [f32], 32, 
  // Allocate scratch registers first, then callee-saved registers
  [T0, T1, T2, T3, T4, T5, K0, K1, GP, AT, //FIXME GP should be reserved under PIC
   A0, A1, A2, A3, A4, A5, A6, A7, V0, V1, 
   S0, S1, S2, S3, S4, S5, S6, S7, //Callee-saved
   ZERO, SP, FP, RA //Reserved
  ]>
{
  let MethodProtos = [{
    iterator allocation_order_end(const MachineFunction &MF) const;
  }];
  let MethodBodies = [{
    FPRegsClass::iterator
    FPRegsClass::allocation_order_end(const MachineFunction &MF) const {
      // The last 4 registers on the list above are reserved, can allocate through S7.
      return end()-4;
    }
  }];
}
