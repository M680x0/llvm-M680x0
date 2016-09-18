//===-- M680x0RegisterInfo.h - M680x0 Register Information Impl --*- C++ --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the M680x0 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0REGISTERINFO_H
#define LLVM_LIB_TARGET_M680X0_M680X0REGISTERINFO_H

#include "M680x0.h"
#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "M680x0GenRegisterInfo.inc"

namespace llvm {
class M680x0Subtarget;
class TargetInstrInfo;
class Type;

class M680x0RegisterInfo : public M680x0GenRegisterInfo {
  virtual void anchor();

  /// StackPtr - M680x0 physical register used as stack ptr.
  unsigned StackPtr;

  /// FramePtr - M680x0 physical register used as frame ptr.
  unsigned FramePtr;

  /// BasePtr - M680x0 physical register used as a base ptr in complex stack
  /// frames. I.e., when we need a 3rd base, not just SP and FP, due to
  /// variable size stack objects.
  unsigned BasePtr;

protected:
  const M680x0Subtarget &Subtarget;

public:
  M680x0RegisterInfo(const M680x0Subtarget &Subtarget);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const override;

  /// getRegsForTailCall - Returns a register class with registers that can be
  /// used in forming tail calls.
  const TargetRegisterClass * getRegsForTailCall(
                                              const MachineFunction &MF) const;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override;

  bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const override;

  /// Stack Frame Processing Methods
  void eliminateFrameIndex(MachineBasicBlock::iterator II,
                           int SPAdj, unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  bool hasBasePointer(const MachineFunction &MF) const;

  /// True if the stack can be realigned for the target.
  bool canRealignStack(const MachineFunction &MF) const override;

  unsigned getFrameRegister(const MachineFunction &MF) const override;
  unsigned getStackRegister() const { return StackPtr; }
  unsigned getBaseRegister() const { return BasePtr; }

  const TargetRegisterClass *intRegClass(unsigned Size) const;
};

} // end namespace llvm

#endif
