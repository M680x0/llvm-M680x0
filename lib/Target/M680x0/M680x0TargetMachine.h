//===-- M680x0TargetMachine.h - Define TargetMachine for M680x0 ----- C++ -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the M680x0 specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0TARGETMACHINE_H
#define LLVM_LIB_TARGET_M680X0_M680X0TARGETMACHINE_H

#include "MCTargetDesc/M680x0MCTargetDesc.h"
#include "M680x0Subtarget.h"

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class formatted_raw_ostream;
class M680x0RegisterInfo;

class M680x0TargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  M680x0Subtarget Subtarget;

  mutable StringMap<std::unique_ptr<M680x0Subtarget>> SubtargetMap;
public:
  M680x0TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                   StringRef FS, const TargetOptions &Options,
                   Optional<Reloc::Model> RM, CodeModel::Model CM,
                   CodeGenOpt::Level OL);

  ~M680x0TargetMachine() override;

  const M680x0Subtarget *getSubtargetImpl() const {
    return &Subtarget;
  }

  const M680x0Subtarget *getSubtargetImpl(const Function &F) const override;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};
} // End llvm namespace

#endif
