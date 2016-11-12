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

M680x0Subtarget::
M680x0Subtarget(const Triple &TT, const std::string &CPU,
                const std::string &FS,
                const M680x0TargetMachine &_TM) :
  M680x0GenSubtargetInfo(TT, CPU, FS),
  PICStyle(PICStyles::None), TM(_TM), TSInfo(),
  InstrInfo(initializeSubtargetDependencies(CPU, FS, TM)),
  FrameLowering(*this, this->getStackAlignment()),
  TLInfo(TM, *this), TargetTriple(TT)  {
  // Determine the PICStyle based on the target selected.
  if (isPositionIndependent())
    // NOTE this limits offsets to 16bit. Newer CPUs can do 32bit
    // TODO make it depend on code model(small, large etc)
    setPICStyle(PICStyles::PCRel);
  else
    setPICStyle(PICStyles::None);
}

bool M680x0Subtarget::
isPositionIndependent() const { return TM.isPositionIndependent(); }

bool M680x0Subtarget::
isLegalToCallImmediateAddr() const {
  return true;
  // return isTargetELF() || TM.getRelocationModel() == Reloc::Static;
}


/// Classify a blockaddress reference for the current subtarget according to how
/// we should reference it in a non-pcrel context.
unsigned char M680x0Subtarget::classifyBlockAddressReference() const {
  return classifyLocalReference(nullptr);
}

unsigned char M680x0Subtarget::
classifyLocalReference(const GlobalValue *GV) const {
  // Use %pc addressing for anything local
  return M680x0II::MO_NO_FLAG;
}

unsigned char M680x0Subtarget::
classifyGlobalReference(const GlobalValue *GV) const {
  return classifyGlobalReference(GV, *GV->getParent());
}

unsigned char M680x0Subtarget::
classifyGlobalReference(const GlobalValue *GV, const Module &M) const {
  // Large model never uses stubs.
  if (TM.getCodeModel() == CodeModel::Large)
    return M680x0II::MO_NO_FLAG;

  if (TM.shouldAssumeDSOLocal(M, GV))
    return classifyLocalReference(GV);

  return M680x0II::MO_GOTPCREL;
}

unsigned char M680x0Subtarget::
classifyGlobalFunctionReference(const GlobalValue *GV) const {
  return classifyGlobalFunctionReference(GV, *GV->getParent());
}

unsigned char M680x0Subtarget::
classifyGlobalFunctionReference(const GlobalValue *GV, const Module &M) const {
  if (TM.shouldAssumeDSOLocal(M, GV))
    return M680x0II::MO_NO_FLAG;

  if (isTargetELF())
    return M680x0II::MO_PLT;

  auto *F = dyn_cast_or_null<Function>(GV);
  if (F && F->hasFnAttribute(Attribute::NonLazyBind)) {
    // If the function is marked as non-lazy, generate an indirect call
    // which loads from the GOT directly. This avoids runtime overhead
    // at the cost of eager binding.
    return M680x0II::MO_GOTPCREL;
  }

  return M680x0II::MO_NO_FLAG;
}

M680x0Subtarget & M680x0Subtarget::
initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                const M680x0TargetMachine &TM) {
  std::string CPUName = selectM680x0CPU(TargetTriple, CPU);

  // Parse features string.
  ParseSubtargetFeatures(CPUName, FS);
  // Initialize scheduling itinerary for the specified CPU.
  InstrItins = getInstrItineraryForCPU(CPUName);

  // Default stack alignment is 8 bytes, ??? Do I need this override?
  // if (StackAlignOverride)
  //   stackAlignment = StackAlignOverride;
  // else
    stackAlignment = 8;

  return *this;
}

bool M680x0Subtarget::abiUsesSoftFloat() const {
//  return TM->Options.UseSoftFloat;
  return true;
}
