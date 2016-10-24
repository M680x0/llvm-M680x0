//===-- M680x0MCTargetDesc.cpp - M680x0 Target Descriptions ---------------===//
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

#include "M680x0MCTargetDesc.h"
#include "InstPrinter/M680x0InstPrinter.h"
#include "M680x0MCAsmInfo.h"

#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "M680x0GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "M680x0GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "M680x0GenRegisterInfo.inc"

static StringRef ParseM680x0Triple(const Triple &TT, StringRef CPU) {
  std::string FS = "";
  return FS;
}

static MCInstrInfo *createM680x0MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitM680x0MCInstrInfo(X); // defined in M680x0GenInstrInfo.inc
  return X;
}

static MCRegisterInfo *createM680x0MCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitM680x0MCRegisterInfo(X, M680x0::PC);
  return X;
}

static MCSubtargetInfo *createM680x0MCSubtargetInfo(const Triple &TT,
                                                  StringRef CPU, StringRef FS) {
  std::string ArchFS = ParseM680x0Triple(TT,CPU);
  if (!FS.empty()) {
    if (!ArchFS.empty())
      ArchFS = ArchFS + "," + FS.str();
    else
      ArchFS = FS;
  }
  return createM680x0MCSubtargetInfoImpl(TT, CPU, ArchFS);
}

static MCAsmInfo *createM680x0MCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT) {
  MCAsmInfo *MAI = new M680x0ELFMCAsmInfo(TT);

  // Initialize initial frame state.
  // Calculate amount of bytes used for return address storing
  int stackGrowth = -4;

  // Initial state of the frame pointer is SP+stackGrowth.
  MCCFIInstruction Inst = MCCFIInstruction::createDefCfa(
    nullptr, MRI.getDwarfRegNum(M680x0::SP, true), -stackGrowth);
  MAI->addInitialFrameState(Inst);

  // Add return address to move list
  MCCFIInstruction Inst2 = MCCFIInstruction::createOffset(
    nullptr, MRI.getDwarfRegNum(M680x0::PC, true), stackGrowth);
  MAI->addInitialFrameState(Inst2);

  return MAI;
}

static MCRelocationInfo *createM680x0MCRelocationInfo(const Triple &TheTriple,
                                                   MCContext &Ctx) {
  // Default to the stock relocation info.
  return llvm::createMCRelocationInfo(TheTriple, Ctx);
}

static MCInstPrinter *createM680x0MCInstPrinter(const Triple &T,
                                                unsigned SyntaxVariant,
                                                const MCAsmInfo &MAI,
                                                const MCInstrInfo &MII,
                                                const MCRegisterInfo &MRI) {
  return new M680x0InstPrinter(MAI, MII, MRI);
}

extern "C" void LLVMInitializeM680x0TargetMC() {
    Target *T = &TheM680x0Target;

    // Register the MC asm info.
    RegisterMCAsmInfoFn X(*T, createM680x0MCAsmInfo);

    // Register the MC instruction info.
    TargetRegistry::RegisterMCInstrInfo(*T, createM680x0MCInstrInfo);

    // Register the MC register info.
    TargetRegistry::RegisterMCRegInfo(*T, createM680x0MCRegisterInfo);

    // Register the MC subtarget info.
    TargetRegistry::RegisterMCSubtargetInfo(*T, createM680x0MCSubtargetInfo);

    // Register the code emitter.
    TargetRegistry::RegisterMCCodeEmitter(*T, createM680x0MCCodeEmitter);

    // Register the MCInstPrinter.
    TargetRegistry::RegisterMCInstPrinter(*T, createM680x0MCInstPrinter);

    // Register the MC relocation info.
    TargetRegistry::RegisterMCRelocationInfo(*T, createM680x0MCRelocationInfo);

    // Register the asm backend.
    TargetRegistry::RegisterMCAsmBackend(TheM680x0Target, createM680x0AsmBackend);
}
