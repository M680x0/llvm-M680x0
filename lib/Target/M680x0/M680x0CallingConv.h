//=== M680x0CallingConv.h - M680x0 Custom Calling Convention Routines  C++ ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the custom routines for the M680x0 Calling Convention that
// aren't done by tablegen.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0CALLINGCONV_H
#define LLVM_LIB_TARGET_M680X0_M680X0CALLINGCONV_H

#include "MCTargetDesc/M680x0MCTargetDesc.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/IR/CallingConv.h"

namespace llvm {

inline bool CC_M680x0_Any_AssignToReg(unsigned &ValNo, MVT &ValVT,
                                       MVT &LocVT,
                                       CCValAssign::LocInfo &LocInfo,
                                       ISD::ArgFlagsTy &ArgFlags,
                                       CCState &State) {
  static const MCPhysReg DataRegList[] = {
    M680x0::D0, M680x0::D1,
    M680x0::A0, M680x0::A1
  };

  static const MCPhysReg AddrRegList[] = {
    M680x0::A0, M680x0::A1
  };

  // SHIT rewrite this
  // NOTE This is probably wrong
  auto I = State.getMachineFunction().getFunction()->arg_begin();
  auto No = ValNo;
  while (No--) {
    I++;
  }

  bool isPtr = I->getType()->isPointerTy();

  unsigned Reg = isPtr ?
    State.AllocateReg(AddrRegList)
    : State.AllocateReg(DataRegList);

  if (Reg) {
    State.addLoc(CCValAssign::getReg(ValNo, ValVT, Reg, LocVT, LocInfo));
    return true;
  }

  return false;
}

} // End llvm namespace

#endif

