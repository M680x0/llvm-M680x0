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

#define GET_INSTRINFO_MI_OPS_INFO
#include "M680x0GenInstrInfo.inc"

namespace llvm {

namespace M680x0 {
  // Enums for memory operand decoding. Supports these forms:
  // (d,An)
  // (d,An,Xn)
  // ([bd,An],Xn,od)
  // ([bd,An,Xn],od)
  enum {
    MemDisp = 0,
    MemBase = 1,
    MemIndex = 2, //FIXME assumes Scale 1 for now
    MemOuter = 3
  };

  // Enums for pc-relative memory operand decoding. Supports these forms:
  // (d,PC)
  // (d,PC,Xn)
  // ([bd,PC],Xn,od)
  // ([bd,PC,Xn],od)
  enum {
    PCRelDisp = 0,
    PCRelIndex = 1,
    PCRelOuter = 2
  };
} // end namespace M680x0;

namespace M680x0Beads {
  enum {
    Ctrl   = 0x0,
    Bits1  = 0x1,
    Bits2  = 0x2,
    Bits3  = 0x3,
    Bits4  = 0x4,
    DAReg  = 0x5,
    DA     = 0x6,
    Reg    = 0x7,
    Disp8  = 0x8,
    Imm8   = 0x9,
    Imm16  = 0xA,
    Imm32  = 0xB,
    Imm3   = 0xC,
  };

  // Ctrl payload
  enum {
    Term   = 0x0,
    Ignore = 0x1,
  };
} /* M680x0Beads */

/// M680x0II - This namespace holds all of the target specific flags that
/// instruction info tracks.
namespace M680x0II {
  /// Target Operand Flag enum.
  enum TOF {
    //===------------------------------------------------------------------===//
    // M680x0 Specific MachineOperand flags.

    MO_NO_FLAG,

    /// MO_ABSOLUTE_ADDRESS - On a symbol operand this indicates that the
    /// immediate is the absolute address of the symbol.
    MO_ABSOLUTE_ADDRESS,

    /// MO_PC_RELATIVE_ADDRESS - On a symbol operand this indicates that the
    /// immediate is the pc-relative address of the symbol.
    MO_PC_RELATIVE_ADDRESS,

    /// MO_GOT - On a symbol operand this indicates that the immediate is the
    /// offset to the GOT entry for the symbol name from the base of the GOT.
    ///
    ///    name@GOT
    MO_GOT,

    /// MO_GOTOFF - On a symbol operand this indicates that the immediate is
    /// the offset to the location of the symbol name from the base of the GOT.
    ///
    ///    name@GOTOFF
    MO_GOTOFF,

    /// MO_GOTPCREL - On a symbol operand this indicates that the immediate is
    /// offset to the GOT entry for the symbol name from the current code
    /// location.
    ///
    ///    name@GOTPCREL
    MO_GOTPCREL,

    /// MO_PLT - On a symbol operand this indicates that the immediate is
    /// offset to the PLT entry of symbol name from the current code location.
    ///
    ///    name@PLT
    MO_PLT,
  }; // enum TOF

  // enum {
  //   //===------------------------------------------------------------------===//
  //   // Instruction encodings.  These are the standard/most common forms for
  //   // M680x0 instructions.
  //   //
  //
  //   // Pseudo - This represents an instruction that is a pseudo instruction
  //   // or one that has not been implemented yet.  It is illegal to code generate
  //   // it, but tolerated for intermediate implementation stages.
  //   Pseudo   = 0,
  //   FormMask = 15
  // };

/// isGlobalStubReference - Return true if the specified TargetFlag operand is
/// a reference to a stub for a global, not the global itself.
inline static bool
isGlobalStubReference(unsigned char TargetFlag) {
  switch (TargetFlag) {
  default: return false;
  case M680x0II::MO_GOTPCREL:  // pc-relative GOT reference.
  case M680x0II::MO_GOT:       // normal GOT reference.
    return true;
  }
}

/// isSimpleGlobalReference - Return True if the specified GlobalValue is a
/// direct reference for a symbol.
inline static bool
isDirectGlobalReference(unsigned char Flag) {
  switch (Flag) {
    default: return false;
    case M680x0II::MO_NO_FLAG:
    case M680x0II::MO_ABSOLUTE_ADDRESS:
    case M680x0II::MO_PC_RELATIVE_ADDRESS:
      return true;
  }
}

/// isGlobalRelativeToPICBase - Return true if the specified global value
/// reference is relative to a 32-bit PIC base (M680x0ISD::GlobalBaseReg). If this
/// is true, the addressing mode has the PIC base register added in.
inline static bool
isGlobalRelativeToPICBase(unsigned char TargetFlag) {
  switch (TargetFlag) {
  default: return false;
  case M680x0II::MO_GOTOFF:          // isPICStyleGOT: local global.
  case M680x0II::MO_GOT:             // isPICStyleGOT: other global.
    return true;
  }
}

/// isPCRelGlobalReference - Return True if the specified GlobalValue requires PC
/// addressing mode.
inline static bool
isPCRelGlobalReference(unsigned char Flag) {
  switch (Flag) {
    default: return false;
    case M680x0II::MO_GOTPCREL:
    case M680x0II::MO_PC_RELATIVE_ADDRESS:
      return true;
  }
}

/// isPCRelBlockReference - Return True if the Block is referenced using PC
inline static bool
isPCRelBlockReference(unsigned char Flag) {
  switch (Flag) {
    default: return false;
    case M680x0II::MO_PC_RELATIVE_ADDRESS:
      return true;
  }
}

static inline bool
isAddressRegister(unsigned RegNo) {
  switch (RegNo) {
    case M680x0::CCR:
    case M680x0::PC:
    case M680x0::SR:
    default:
      llvm_unreachable("Not an Address nor Data register");
    case M680x0::WA0:
    case M680x0::WA1:
    case M680x0::WA2:
    case M680x0::WA3:
    case M680x0::WA4:
    case M680x0::WA5:
    case M680x0::WA6:
    case M680x0::WA7:
    case M680x0::A0:
    case M680x0::A1:
    case M680x0::A2:
    case M680x0::A3:
    case M680x0::A4:
    case M680x0::A5:
    case M680x0::A6:
    // case M680x0::A7:
    case M680x0::SP:
      return true;
    case M680x0::BD0:
    case M680x0::BD1:
    case M680x0::BD2:
    case M680x0::BD3:
    case M680x0::BD4:
    case M680x0::BD5:
    case M680x0::BD6:
    case M680x0::BD7:
    case M680x0::WD0:
    case M680x0::WD1:
    case M680x0::WD2:
    case M680x0::WD3:
    case M680x0::WD4:
    case M680x0::WD5:
    case M680x0::WD6:
    case M680x0::WD7:
    case M680x0::D0:
    case M680x0::D1:
    case M680x0::D2:
    case M680x0::D3:
    case M680x0::D4:
    case M680x0::D5:
    case M680x0::D6:
    case M680x0::D7:
      return false;
  }
}

static inline bool
isPCRelOpd(unsigned Opd) {
  switch (Opd) {
    default: return false;
    case M680x0::MIOpTypes::MxPCD32:
    case M680x0::MIOpTypes::MxPCD16:
    case M680x0::MIOpTypes::MxPCD8:
    case M680x0::MIOpTypes::MxPCI32:
    case M680x0::MIOpTypes::MxPCI16:
    case M680x0::MIOpTypes::MxPCI8:
    case MCOI::OPERAND_PCREL:
      return true;
  }
}

static inline unsigned
getDispSize(unsigned Opd) {
  switch (Opd) {
    default: return 0;
    case M680x0::MIOpTypes::MxAL16:
    case M680x0::MIOpTypes::MxAL32:
    case M680x0::MIOpTypes::MxAL8:
      return 32;
    case M680x0::MIOpTypes::MxARID16:
    case M680x0::MIOpTypes::MxARID16_TC:
    case M680x0::MIOpTypes::MxARID32:
    case M680x0::MIOpTypes::MxARID32_TC:
    case M680x0::MIOpTypes::MxARID8:
    case M680x0::MIOpTypes::MxARID8_TC:
    case M680x0::MIOpTypes::MxPCD16:
    case M680x0::MIOpTypes::MxPCD32:
    case M680x0::MIOpTypes::MxPCD8:
    case M680x0::MIOpTypes::MxAS16:
    case M680x0::MIOpTypes::MxAS32:
    case M680x0::MIOpTypes::MxAS8:
      return 16;
    case M680x0::MIOpTypes::MxARII16:
    case M680x0::MIOpTypes::MxARII16_TC:
    case M680x0::MIOpTypes::MxARII32:
    case M680x0::MIOpTypes::MxARII32_TC:
    case M680x0::MIOpTypes::MxARII8:
    case M680x0::MIOpTypes::MxARII8_TC:
    case M680x0::MIOpTypes::MxPCI16:
    case M680x0::MIOpTypes::MxPCI32:
    case M680x0::MIOpTypes::MxPCI8:
      return 8;
  }
}

static inline unsigned
getMaskedSpillRegister(unsigned order) {
  switch (order) {
    default: return 0;
    case  0: return M680x0::D0;
    case  1: return M680x0::D1;
    case  2: return M680x0::D2;
    case  3: return M680x0::D3;
    case  4: return M680x0::D4;
    case  5: return M680x0::D5;
    case  6: return M680x0::D6;
    case  7: return M680x0::D7;
    case  8: return M680x0::A0;
    case  9: return M680x0::A1;
    case 10: return M680x0::A2;
    case 11: return M680x0::A3;
    case 12: return M680x0::A4;
    case 13: return M680x0::A5;
    case 14: return M680x0::A6;
    case 15: return M680x0::A7;
  }
}

} // end of M680x0II namespace

} // end of llvm namespace

#endif
