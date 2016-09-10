//===-- M680x0FrameLowering.h - Define frame lowering for M680x0 --- C++ --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_M680X0_M680X0FRAMELOWERING_H
#define LLVM_LIB_TARGET_M680X0_M680X0FRAMELOWERING_H

#include "M680x0.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {
  class M680x0Subtarget;

class M680x0FrameLowering : public TargetFrameLowering {
protected:
  const M680x0Subtarget &STI;

public:
  explicit M680x0FrameLowering(const M680x0Subtarget &sti, unsigned Alignment)
    : TargetFrameLowering(StackGrowsDown, Alignment, 0, Alignment),
      STI(sti) {
  }

  static const M680x0FrameLowering *create(const M680x0Subtarget &ST);

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  bool hasFP(const MachineFunction &MF) const override;

};

} // End llvm namespace

#endif
