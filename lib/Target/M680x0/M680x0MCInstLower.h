//===-- M680x0MCInstLower.h - Lower MachineInstr to MCInst -------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0MCINSTLOWER_H
#define LLVM_LIB_TARGET_M680X0_M680X0MCINSTLOWER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/Support/Compiler.h"

namespace llvm {
  class MCContext;
  class MCInst;
  class MCOperand;
  class MachineInstr;
  class MachineFunction;
  class M680x0AsmPrinter;

/// This class is used to lower an MachineInstr into an MCInst.
class M680x0MCInstLower {
  typedef MachineOperand::MachineOperandType MachineOperandType;
  MCContext &Ctx;
  MachineFunction &MF;
  const TargetMachine &TM;
  const MCAsmInfo &MAI;
  M680x0AsmPrinter &AsmPrinter;
public:
  M680x0MCInstLower(MachineFunction &MF, M680x0AsmPrinter &AP);

  /// GetSymbolFromOperand - Lower an MO_GlobalAddress or MO_ExternalSymbol
  /// operand to an MCSymbol.
  MCSymbol *GetSymbolFromOperand(const MachineOperand &MO) const;

  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;

  Optional<MCOperand> LowerOperand(const MachineInstr *MI,
                                   const MachineOperand &MO) const;

  void Lower(const MachineInstr *MI, MCInst &OutMI) const;
};
}

#endif
