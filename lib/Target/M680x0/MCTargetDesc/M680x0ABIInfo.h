//===---- M680x0ABIInfo.h - Information about M680X0 ABI's ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0ABIINFO_H
#define LLVM_LIB_TARGET_M680X0_MCTARGETDESC_M680X0ABIINFO_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/MC/MCRegisterInfo.h"

namespace llvm {

class MCTargetOptions;
class StringRef;
class TargetRegisterClass;

// FIXME this ABI thing is a copy from Cpu0 tutorial, not sure if i need this
// If so it must follow Classic Mac stack- and register-based conventions
class M680x0ABIInfo {
public:
  enum class ABI { Unknown, O32, S32 };

protected:
  ABI ThisABI;

public:
  M680x0ABIInfo(ABI ThisABI) : ThisABI(ThisABI) {}

  static M680x0ABIInfo Unknown() { return M680x0ABIInfo(ABI::Unknown); }
  static M680x0ABIInfo O32()     { return M680x0ABIInfo(ABI::O32); }
  static M680x0ABIInfo S32()     { return M680x0ABIInfo(ABI::S32); }
  static M680x0ABIInfo computeTargetABI();

  bool IsKnown()     const { return ThisABI != ABI::Unknown; }
  bool IsO32()       const { return ThisABI == ABI::O32; }
  bool IsS32()       const { return ThisABI == ABI::S32; }
  ABI GetEnumValue() const { return ThisABI; }

  /// The registers to use for byval arguments.
  const ArrayRef<MCPhysReg> GetByValArgRegs() const;

  /// The registers to use for the variable argument list.
  const ArrayRef<MCPhysReg> GetVarArgRegs() const;

  /// Obtain the size of the area allocated by the callee for arguments.
  /// CallingConv::FastCall affects the value for O32.
  unsigned GetCalleeAllocdArgSizeInBytes(CallingConv::ID CC) const;

  /// Ordering of ABI's
  /// M680x0GenSubtargetInfo.inc will use this to resolve conflicts when given
  /// multiple ABI options.
  bool operator<(const M680x0ABIInfo Other) const {
    return ThisABI < Other.GetEnumValue();
  }

  unsigned GetStackPtr() const;
  unsigned GetFramePtr() const;

  unsigned GetEhDataReg(unsigned I) const;
  int EhDataRegSize() const;
};
}

#endif
