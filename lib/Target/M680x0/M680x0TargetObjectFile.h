//===-- llvm/Target/M680x0ELFTargetObjectFile.h - M680x0 Object Info -- C++ -====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0TARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_M680X0_M680X0TARGETOBJECTFILE_H

#include "M680x0TargetMachine.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm {
class M680x0TargetMachine;
  class M680x0ELFTargetObjectFile : public TargetLoweringObjectFileELF {
    const M680x0TargetMachine *TM;
    MCSection *SmallDataSection;
    MCSection *SmallBSSSection;
  public:
    void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

  };
} // end namespace llvm

#endif
