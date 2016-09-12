//===-- M680x0MCAsmInfo.h - M680x0 Asm Info --------------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the M680x0MCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0MCASMINFO_H
#define LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
  class Triple;

  class M680x0ELFMCAsmInfo : public MCAsmInfoELF {
    void anchor() override;

  public:
    explicit M680x0ELFMCAsmInfo(const Triple &Triple);
  };

} // namespace llvm

#endif
