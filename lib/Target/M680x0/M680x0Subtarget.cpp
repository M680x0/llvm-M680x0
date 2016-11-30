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
                const M680x0TargetMachine &TM) :
  M680x0GenSubtargetInfo(TT, CPU, FS),
  PICStyle(PICStyles::None), TM(TM), TSInfo(),
  InstrInfo(initializeSubtargetDependencies(CPU, FS, TM)),
  FrameLowering(*this, this->getStackAlignment()),
  TLInfo(TM, *this), TargetTriple(TT)  {
  if (isPositionIndependent()) {
    setPICStyle(PICStyles::PCRel);
  } else {
    setPICStyle(PICStyles::None);
  }
}

bool M680x0Subtarget::
isPositionIndependent() const { return TM.isPositionIndependent(); }

bool M680x0Subtarget::
isLegalToCallImmediateAddr() const { return true; }

bool M680x0Subtarget::
abiUsesSoftFloat() const {
//  return TM->Options.UseSoftFloat;
  return true;
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

//===----------------------------------------------------------------------===//
// Code Model
//
// Key assumptions:
//  - Whenever possible we use pc-rel encoding since it is smaller(16 bit) than
//    absolute(32 bit).
//  - GOT is reachable within 16 bit offset for both Small and Medium models.
//  - Code section is reachable within 16 bit offset for both models.
//
//  ---------------------+-------------------------+--------------------------
//                       |          Small          |          Medium          
//                       +-------------------------+------------+-------------
//                       |   Static   |    PIC     |   Static   |    PIC      
//  ---------------------+------------+------------+------------+-------------
//                branch |   pc-rel   |   pc-rel   |   pc-rel   |   pc-rel    
//  ---------------------+------------+------------+------------+-------------
//           call global |    @PLT    |    @PLT    |    @PLT    |    @PLT     
//  ---------------------+------------+------------+------------+-------------
//         call internal |   pc-rel   |   pc-rel   |   pc-rel   |   pc-rel    
//  ---------------------+------------+------------+------------+-------------
//            data local |   pc-rel   |   pc-rel   |  ~pc-rel   |  ^pc-rel 
//  ---------------------+------------+------------+------------+-------------
//       data local big* |   pc-rel   |   pc-rel   |  absolute  |  @GOTOFF
//  ---------------------+------------+------------+------------+-------------
//           data global |   pc-rel   |  @GOTPCREL |  ~pc-rel   |  @GOTPCREL           
//  ---------------------+------------+------------+------------+-------------
//      data global big* |   pc-rel   |  @GOTPCREL |  absolute  |  @GOTPCREL
//  ---------------------+------------+------------+------------+-------------
//
// * Big data potentially cannot be reached within 16 bit offset and requires
//   special handling for old(x00 and x10) CPUs. Normally these symbols go into
//   separate .ldata section which mapped after normal .data and .text, but I
//   don't really know how this must be done for M680x0 atm... will try to dig
//   this info out from GCC. For now CPUs prior to M68020 will use static ref
//   for Static Model and @GOT based references for PIC.
//
// ~ These are absolute for older CPUs for now.
// ^ These are @GOTOFF for older CPUs for now.
//===----------------------------------------------------------------------===//

/// Classify a blockaddress reference for the current subtarget according to how
/// we should reference it in a non-pcrel context.
unsigned char M680x0Subtarget::classifyBlockAddressReference() const {
  // Unless we start to support Large Code Model branching is always pc-rel
  return M680x0II::MO_PC_RELATIVE_ADDRESS;
}

unsigned char M680x0Subtarget::
classifyLocalReference(const GlobalValue *GV) const {
  switch (TM.getCodeModel()) {
    default: llvm_unreachable("Unsupported code model");
    case CodeModel::Small:
    case CodeModel::Kernel: {
      return M680x0II::MO_PC_RELATIVE_ADDRESS;
    }
    case CodeModel::Medium: {
      if (isPositionIndependent()) {
        // On M68020 and better we can fit big any data offset into dips field.
        if (IsM68020) {
          return M680x0II::MO_PC_RELATIVE_ADDRESS;
        }
        // Otherwise we could check the data size and make sure it will fit into
        // 16 bit offset. For now we will be conservative and go with @GOTOFF
        return M680x0II::MO_GOTOFF;
      } else {
        if (IsM68020) {
          return M680x0II::MO_PC_RELATIVE_ADDRESS;
        }
        return M680x0II::MO_ABSOLUTE_ADDRESS;
      }
    }
  }
}

unsigned char M680x0Subtarget::
classifyExternalReference(const Module &M) const {
  if (TM.shouldAssumeDSOLocal(M, nullptr))
    return classifyLocalReference(nullptr);

  if (isPositionIndependent()) {
    return M680x0II::MO_GOTPCREL;
  } else {
    return M680x0II::MO_GOT;
  }
}

unsigned char M680x0Subtarget::
classifyGlobalReference(const GlobalValue *GV) const {
  return classifyGlobalReference(GV, *GV->getParent());
}

unsigned char M680x0Subtarget::
classifyGlobalReference(const GlobalValue *GV, const Module &M) const {
  if (TM.shouldAssumeDSOLocal(M, GV))
    return classifyLocalReference(GV);

  switch (TM.getCodeModel()) {
    default: llvm_unreachable("Unsupported code model");
    case CodeModel::Small:
    case CodeModel::Kernel: {
      if (isPositionIndependent()) {
        return M680x0II::MO_GOTPCREL;
      } else {
        return M680x0II::MO_PC_RELATIVE_ADDRESS;
      }
    }
    case CodeModel::Medium: {
      if (isPositionIndependent()) {
        return M680x0II::MO_GOTPCREL;
      } else {
        if (IsM68020) {
          return M680x0II::MO_PC_RELATIVE_ADDRESS;
        }
        return M680x0II::MO_ABSOLUTE_ADDRESS;
      }
    }
  }
}

unsigned char M680x0Subtarget::
classifyGlobalFunctionReference(const GlobalValue *GV) const {
  return classifyGlobalFunctionReference(GV, *GV->getParent());
}

unsigned char M680x0Subtarget::
classifyGlobalFunctionReference(const GlobalValue *GV, const Module &M) const {
  // local always use pc-rel referencing
  if (TM.shouldAssumeDSOLocal(M, GV))
    return M680x0II::MO_NO_FLAG;

  // If the function is marked as non-lazy, generate an indirect call
  // which loads from the GOT directly. This avoids runtime overhead
  // at the cost of eager binding.
  auto *F = dyn_cast_or_null<Function>(GV);
  if (F && F->hasFnAttribute(Attribute::NonLazyBind)) {
    return M680x0II::MO_GOTPCREL;
  }

  // otherwise linker will figure this out
  return M680x0II::MO_PLT;
}
