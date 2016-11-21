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
    MemScale = 2, //FIXME assumes Scale 1 for now
    MemOuter = 3
  };

  // Enums for pc-relative memory operand decoding. Supports these forms:
  // (d,PC)
  // (d,PC,Xn)
  // ([bd,PC],Xn,od)
  // ([bd,PC,Xn],od)
  enum { 
    PCRelDisp = 0,
    PCRelScale = 1,
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

    /// MO_GOT_ABSOLUTE_ADDRESS - On a symbol operand, this represents a
    /// relocation of:
    ///    SYMBOL_LABEL + [. - PICBASELABEL]
    MO_GOT_ABSOLUTE_ADDRESS,

    /// MO_PIC_BASE_OFFSET - On a symbol operand this indicates that the
    /// immediate should get the value of the symbol minus the PIC base label:
    ///    SYMBOL_LABEL - PICBASELABEL
    MO_PIC_BASE_OFFSET,

    /// MO_GOT - On a symbol operand this indicates that the immediate is the
    /// offset to the GOT entry for the symbol name from the base of the GOT.
    ///
    /// TODO Read through M680x0 ELF ABI
    ///    SYMBOL_LABEL @GOT
    MO_GOT,

    /// MO_GOTOFF - On a symbol operand this indicates that the immediate is
    /// the offset to the location of the symbol name from the base of the GOT.
    ///
    /// TODO Read through M680x0 ELF ABI
    ///    SYMBOL_LABEL @GOTOFF
    MO_GOTOFF,

    /// MO_GOTPCREL - On a symbol operand this indicates that the immediate is
    /// offset to the GOT entry for the symbol name from the current code
    /// location.
    ///
    /// TODO Read through M680x0 ELF ABI
    ///    SYMBOL_LABEL @GOTPCREL
    MO_GOTPCREL,

    /// MO_PLT - On a symbol operand this indicates that the immediate is
    /// offset to the PLT entry of symbol name from the current code location.
    ///
    /// TODO Read through M680x0 ELF ABI
    ///    SYMBOL_LABEL @PLT
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

} // end of M680x0II namespace

} // end of llvm namespace

#endif
