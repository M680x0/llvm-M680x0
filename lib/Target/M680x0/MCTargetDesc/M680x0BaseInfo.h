//===-- M680x0BaseInfo.h - Top level definitions for M680X0 MC --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the M680x0 target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0BASEINFO_H
#define LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0BASEINFO_H

#include "M680x0MCTargetDesc.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

// TODO fill the data
namespace M680x0 {
  // Enums for memory operand decoding.
} // end namespace M680x0;

/// M680x0II - This namespace holds all of the target specific flags that
/// instruction info tracks.
namespace M680x0II {
  /// Target Operand Flag enum.
  enum TOF {
    //===------------------------------------------------------------------===//
    // M680x0 Specific MachineOperand flags.

    MO_NO_FLAG,
  }; // enum TOF {

  enum {
    //===------------------------------------------------------------------===//
    // Instruction encodings.  These are the standard/most common forms for
    // M680x0 instructions.
    //

    // Pseudo - This represents an instruction that is a pseudo instruction
    // or one that has not been implemented yet.  It is illegal to code generate
    // it, but tolerated for intermediate implementation stages.
    Pseudo   = 0,
    FormMask = 15
  };
}

}

#endif
