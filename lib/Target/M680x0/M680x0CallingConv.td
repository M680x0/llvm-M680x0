//===-- M680X0CallingConv.td - Calling Conventions M680X0 tablegen --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This describes the calling conventions for the M680x0 architectures.
// These conventions assume Int to be 4 bytes and 4 byte aligned. Short
// variant is not supported yet.
//
//===----------------------------------------------------------------------===//

class CCIfSubtarget<string F, CCAction A>
    : CCIf<!strconcat("static_cast<const M680X0Subtarget&>"
                       "(State.getMachineFunction().getSubtarget()).", F), A>;

// TODO Define Stack-based and Register based Classic Mac CC

//===----------------------------------------------------------------------===//
// Return Value Calling Conventions
//===----------------------------------------------------------------------===//

// Return-value conventions common to all M680x0 CC's.
def RetCC_M680x0Common : CallingConv<[
  CCIfType<[i1],   CCPromoteToType<i8>>,
  CCIfType<[i8],   CCAssignToReg<[D0, D1]>>,
  CCIfType<[i16],  CCAssignToReg<[D0, D1]>>,
  // Allows up to 16 bytes returned in registers
  CCIfType<[i32],  CCAssignToReg<[D0, D1, A0, A1]>>,
  /* CCIfType<[iPTR], CCAssignToReg<[A0, A1]>>, */
  // It seems GCC does this so i will follow
  CCIfType<[iPTR], CCAssignToReg<[D0, D1]>>
]>;

// M680x0 C return-value convention.
def RetCC_M680x0_C : CallingConv<[
  /* CCIfType<[f32,f64], CCAssignToReg<[FP0, FP1]>>, */
  CCDelegateTo<RetCC_M680x0Common>
]>;

// This is the root return-value convention for the M680x0 backend.
def RetCC_M680x0 : CallingConv<[
  // TODO delegate to Classic Mac's CC
  // Otherwise, drop to normal M680x0 CC
  CCDelegateTo<RetCC_M680x0_C>
]>;

//===----------------------------------------------------------------------===//
// M680x0 C Calling Convention
//===----------------------------------------------------------------------===//

/// CC_M680x0_Common - In all M680x0 calling conventions, extra integers and FP
/// values are spilled on the stack.
def CC_M680x0_Common : CallingConv<[
  // Handles byval parameters.
  CCIfByVal<CCPassByVal<4, 4>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32],  CCAssignToStack<4, 4>>,
  CCIfType<[iPTR], CCAssignToStack<4, 4>>
]>;

def CC_M680x0_C : CallingConv<[
  // Promote i1/i8/i16 arguments to i32.
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,

  // The 'nest' parameter, if any, is passed in A1.
  CCIfNest<CCAssignToReg<[A1]>>, // FIXME is this correct?

  // The first 2 integer/ptr arguments, if marked 'inreg' and if the call is not
  // a vararg call, are passed in integer registers.
  CCIfNotVarArg<CCIfInReg<CCIfType<[i32],  CCAssignToReg<[D0, D1]>>>>,
  CCIfNotVarArg<CCIfInReg<CCIfType<[iPTR], CCAssignToReg<[A0, A1]>>>>,

  // Otherwise, same as everything else.
  CCDelegateTo<CC_M680x0_Common>
]>;

// This is the root argument convention for the M680x0 backend.
def CC_M680x0 : CallingConv<[
  // TODO delegate to Classic Mac's CC
  // Otherwise, drop to normal M680x0 CC
  CCDelegateTo<CC_M680x0_C>
]>;

//===----------------------------------------------------------------------===//
// Callee-saved Registers.
//===----------------------------------------------------------------------===//

def CSR_NoRegs : CalleeSavedRegs<(add)>;

// A5 - DP
// A6 - FP
// A7 - SP
// TODO callee save/restore should collapse into a single MOVEM instruction
def CSR_STD : CalleeSavedRegs<(add D2, D3, D4, D5, D6, D7,
                                   A2, A3, A4, A5, A6, A7)>;

