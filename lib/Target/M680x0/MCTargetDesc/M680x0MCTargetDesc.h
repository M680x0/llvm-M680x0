//===-- M680x0MCTargetDesc.h - M680x0 Target Descriptions -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides M680x0 specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_CPU0_MCTARGETDESC_CPU0MCTARGETDESC_H
#define LLVM_LIB_TARGET_CPU0_MCTARGETDESC_CPU0MCTARGETDESC_H

#include "llvm/Support/DataTypes.h"

namespace llvm {
class Target;
class Triple;

extern Target TheM680x0Target;
extern Target TheM680x0elTarget;

} // End llvm namespace

// Defines symbolic names for M680x0 registers. This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "M680x0GenRegisterInfo.inc"

// Defines symbolic names for the M680x0 instructions.
#define GET_INSTRINFO_ENUM
#include "M680x0GenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "M680x0GenSubtargetInfo.inc"

#endif
