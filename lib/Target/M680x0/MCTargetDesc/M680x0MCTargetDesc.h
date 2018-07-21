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

#ifndef LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0MCTARGETDESC_H
#define LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0MCTARGETDESC_H

#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCRelocationInfo;
class MCTargetOptions;
class Target;
class Triple;
class StringRef;
class raw_ostream;
class raw_pwrite_stream;

extern Target TheM680x0Target;

MCAsmBackend *createM680x0AsmBackend(const Target &T, const MCRegisterInfo &MRI,
                                     const Triple &TT, StringRef CPU,
                                     const MCTargetOptions &Options);

MCCodeEmitter *createM680x0MCCodeEmitter(const MCInstrInfo &MCII,
                                         const MCRegisterInfo &MRI,
                                         MCContext &Ctx);

/// Construct an M680x0 ELF object writer.
std::unique_ptr<MCObjectWriter>
createM680x0ELFObjectWriter(raw_pwrite_stream &OS, uint8_t OSABI);

} // namespace llvm

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
