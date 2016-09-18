//===-- M680x0Subtarget.cpp - M680x0 Subtarget Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the M680x0 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "M680x0Subtarget.h"

#include "M680x0MachineFunction.h"
#include "M680x0.h"
#include "M680x0RegisterInfo.h"

#include "M680x0TargetMachine.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "m680x0-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "M680x0GenSubtargetInfo.inc"

extern bool FixGlobalBaseReg;

/// Select the M680x0 CPU for the given triple and cpu name.
/// FIXME: Merge with the copy in M680x0MCTargetDesc.cpp
static StringRef selectM680x0CPU(Triple TT, StringRef CPU) {
  if (CPU.empty() || CPU == "generic") {
      CPU = "M68000";
  }
  return CPU;
}

void M680x0Subtarget::anchor() { }

M680x0Subtarget::M680x0Subtarget(const Triple &TT, const std::string &CPU,
                             const std::string &FS, 
                             const M680x0TargetMachine &_TM) :
  // M680x0GenSubtargetInfo will display features by llc -march=cpu0 -mcpu=help
  M680x0GenSubtargetInfo(TT, CPU, FS),
  TM(_TM), TargetTriple(TT), TSInfo(),
  InstrInfo(initializeSubtargetDependencies(CPU, FS, TM)),
  FrameLowering(*this, this->getStackAlignment()),
  TLInfo(TM, *this) { }

M680x0Subtarget &
M680x0Subtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                               const M680x0TargetMachine &TM) {
  std::string CPUName = selectM680x0CPU(TargetTriple, CPU);

  // Parse features string.
  ParseSubtargetFeatures(CPUName, FS);
  // Initialize scheduling itinerary for the specified CPU.
  InstrItins = getInstrItineraryForCPU(CPUName);

  return *this;
}

bool M680x0Subtarget::abiUsesSoftFloat() const {
//  return TM->Options.UseSoftFloat;
  return true;
}

const M680x0ABIInfo &M680x0Subtarget::getABI() const { return TM.getABI(); }
