//===-- M680x0AsmPrinter.cpp - M680x0 LLVM Assembly Printer ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format M680x0 assembly language.
//
//===----------------------------------------------------------------------===//

// TODO make it print Motorola asm
//
#include "M680x0AsmPrinter.h"

#include "InstPrinter/M680x0InstPrinter.h"
#include "MCTargetDesc/M680x0BaseInfo.h"
#include "M680x0.h"
#include "M680x0InstrInfo.h"
#include "M680x0MachineFunction.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

#define DEBUG_TYPE "m680x0-asm-printer"

bool M680x0AsmPrinter::
runOnMachineFunction(MachineFunction &MF) {
  MMFI = MF.getInfo<M680x0MachineFunctionInfo>();
  MCInstLowering = make_unique<M680x0MCInstLower>(MF, *this);
  AsmPrinter::runOnMachineFunction(MF);
  return true;
}

void M680x0AsmPrinter::
EmitInstruction(const MachineInstr *MI) {
  switch (MI->getOpcode()) {
  default: {
    if (MI->isPseudo()) {
      DEBUG(dbgs()
          << "Pseudo opcode("
          << MI->getOpcode()
          << ") found in EmitInstruction()\n");
      llvm_unreachable("Cannot proceed");
    }
    break;
  }
  case M680x0::TAILJMPj:
  case M680x0::TAILJMPq:
    // Lower these as normal, but add some comments.
    OutStreamer->AddComment("TAILCALL");
    break;
  }

  MCInst TmpInst0;
  MCInstLowering->Lower(MI, TmpInst0);
  OutStreamer->EmitInstruction(TmpInst0, getSubtargetInfo());
}

void M680x0AsmPrinter::
EmitFunctionBodyStart() {
  // TODO
}

void M680x0AsmPrinter::
EmitFunctionBodyEnd() {
  // TODO
}

void M680x0AsmPrinter::
EmitStartOfAsmFile(Module &M) {
  OutStreamer->EmitSyntaxDirective();
}

void M680x0AsmPrinter::
EmitEndOfAsmFile(Module &M) {
}

extern "C" void LLVMInitializeM680x0AsmPrinter() {
  RegisterAsmPrinter<M680x0AsmPrinter> X(TheM680x0Target);
}
