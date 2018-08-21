//===-- M680x0AsmPrinter.h - M680x0 LLVM Assembly Printer -------- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// M680x0 Assembly printer class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0ASMPRINTER_H
#define LLVM_LIB_TARGET_M680X0_M680X0ASMPRINTER_H

#include "M680x0MCInstLower.h"
#include "M680x0TargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class MCStreamer;
class MachineInstr;
class MachineBasicBlock;
class Module;
class raw_ostream;

class M680x0Subtarget;
class M680x0MachineFunctionInfo;

class LLVM_LIBRARY_VISIBILITY M680x0AsmPrinter : public AsmPrinter {

  void EmitInstrWithMacroNoAT(const MachineInstr *MI);

public:
  const M680x0Subtarget *Subtarget;
  const M680x0MachineFunctionInfo *MMFI;
  std::unique_ptr<M680x0MCInstLower> MCInstLowering;

  explicit M680x0AsmPrinter(TargetMachine &TM,
                            std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {
    Subtarget = static_cast<M680x0TargetMachine &>(TM).getSubtargetImpl();
  }

  StringRef getPassName() const override { return "M680x0 Assembly Printer"; }

  virtual bool runOnMachineFunction(MachineFunction &MF) override;

  void EmitInstruction(const MachineInstr *MI) override;
  void EmitFunctionBodyStart() override;
  void EmitFunctionBodyEnd() override;
  void EmitStartOfAsmFile(Module &M) override;
  void EmitEndOfAsmFile(Module &M) override;
};
} // namespace llvm

#endif
