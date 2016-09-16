//===-- M680x0TargetMachine.cpp - Define TargetMachine for M680x0 ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about M680x0 target spec.
//
//===----------------------------------------------------------------------===//

#include "M680x0TargetMachine.h"
#include "M680x0.h"

#include "M680x0Subtarget.h"
#include "M680x0TargetObjectFile.h"
#include "M680x0ISelDAGToDAG.h"

#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

#define DEBUG_TYPE "m680x0"

extern "C" void LLVMInitializeM680x0Target() {
  RegisterTargetMachine<M680x0TargetMachine> X(TheM680x0Target);
}

// FIXME this layout is true for M68000 original cpu, other variants will
// affect DL computation
static std::string computeDataLayout(const Triple &TT, StringRef CPU,
                                     const TargetOptions &Options) {
  std::string Ret = "";
  // M680x0 is Big Endian
  Ret += "E";

  // FIXME how to wire it with the used object format?
  Ret += "-m:e";

  // M680x0 pointers are always 32 bit wide even for 16 bit cpus
  Ret += "-p:32:32";

  // M680x0 requires i8 to align on 2 byte boundry
  Ret += "-i8:16:16-i16:16:16-i32:32:32";

  // FIXME no floats at the moment

  // The registers can hold 8, 16, 32 bits
  Ret += "-n8:16:32";

  // Aggregates are 32 bit aligned and stack is 16 bit aligned
  Ret += "-a:0:32-S16";

  return Ret;
}

static Reloc::Model getEffectiveRelocModel(const Triple &TT,
                                           Optional<Reloc::Model> RM) {
    // If not defined we default to static
  if (!RM.hasValue()) {
    return Reloc::Static;
  }

  return *RM;
}

M680x0TargetMachine::M680x0TargetMachine(const Target &T, const Triple &TT,
                                   StringRef CPU, StringRef FS,
                                   const TargetOptions &Options,
                                   Optional<Reloc::Model> RM,
                                   CodeModel::Model CM, CodeGenOpt::Level OL)
    : LLVMTargetMachine(T, computeDataLayout(TT, CPU, Options), TT, CPU, FS,
                        Options, getEffectiveRelocModel(TT, RM), CM, OL),
      TLOF(make_unique<M680x0ELFTargetObjectFile>()),
      ABI(M680x0ABIInfo::computeTargetABI()),
      Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();
}

M680x0TargetMachine::~M680x0TargetMachine() {}

const M680x0Subtarget *
M680x0TargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU = !CPUAttr.hasAttribute(Attribute::None)
                        ? CPUAttr.getValueAsString().str()
                        : TargetCPU;
  std::string FS = !FSAttr.hasAttribute(Attribute::None)
                       ? FSAttr.getValueAsString().str()
                       : TargetFS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = llvm::make_unique<M680x0Subtarget>(TargetTriple, CPU, FS, *this);
  }
  return I.get();
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

namespace {
class M680x0PassConfig : public TargetPassConfig {
public:
  M680x0PassConfig(M680x0TargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  M680x0TargetMachine &getM680x0TargetMachine() const {
    return getTM<M680x0TargetMachine>();
  }

  const M680x0Subtarget &getM680x0Subtarget() const {
    return *getM680x0TargetMachine().getSubtargetImpl();
  }

  bool addInstSelector() override;
};
} // namespace

TargetPassConfig *M680x0TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new M680x0PassConfig(this, PM);
}

bool M680x0PassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createM680x0ISelDag(getM680x0TargetMachine()));
  return false;
}
