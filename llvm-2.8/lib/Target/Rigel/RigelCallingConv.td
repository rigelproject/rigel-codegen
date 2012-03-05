//===- RigelCallingConv.td - Calling Conventions for Rigel --------*- C++ -*-===//
// 
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the Rigel Team and is distributed under the 
// University of Illinois Open Source License. See LICENSE.TXT for details.
// 
//===----------------------------------------------------------------------===//
// This describes the calling conventions for Rigel architecture.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Rigel Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_Rigel : CallingConv<[
  // i32 are returned in registers V0, V1
  CCIfType<[i32, f32], CCAssignToReg<[V0, V1]>>
]>;

//===----------------------------------------------------------------------===//
// Rigel Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_Rigel : CallingConv<[
  // Promote i8/i16 arguments to i32.
  CCIfType<[i8, i16], CCPromoteToType<i32>>,

  CCIfType<[i32, f32], CCAssignToReg<[A0, A1, A2, A3, A4, A5, A6, A7]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32, f32], CCAssignToStack<4, 4>>
]>;

