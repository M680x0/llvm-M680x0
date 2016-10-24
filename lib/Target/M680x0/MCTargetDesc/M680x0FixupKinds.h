//===-- M680x0FixupKinds.h - M680x0 Specific Fixup Entries ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680x0_MCTARGETDESC_M680x0FIXUPKINDS_H
#define LLVM_LIB_TARGET_M680x0_MCTARGETDESC_M680x0FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace M680x0 {
enum Fixups {
  // Marker
  LastTargetFixupKind,
  NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
};
}
}

#endif
