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
#include "llvm/CodeGen/TargetRegisterInfo.h"

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

  /// GlobalBasePtr - Physical register used to store GOT address if needed.
  unsigned GlobalBasePtr;

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

  /// Return a mega-register of the specified register Reg so its sub-register
  /// of index SubIdx is Reg, its super(or mega) Reg. In other words it will
  /// return a register that is not direct super register but still shares
  /// physical register with Reg.
  /// NOTE not sure about the term though.
  unsigned getMatchingMegaReg(unsigned Reg, const TargetRegisterClass *RC) const;

  /// getMaximalPhysRegClass - Returns the Register Class of a physical
  /// register of the given type, picking the biggest register class of
  /// the right type that contains this physreg.
  const TargetRegisterClass * getMaximalPhysRegClass(unsigned reg, MVT VT) const;

  /// Return index of a register within a register class, otherwise return -1
  int getRegisterOrder(unsigned Reg, const TargetRegisterClass &TRC) const;

  /// Return spill order index of a register, if there is none then trap
  int getSpillRegisterOrder(unsigned Reg) const;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool requiresRegisterScavenging(const MachineFunction &MF) const override;

  bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const override;

  // FrameIndex represent objects inside a abstract stack. We must replace
  // FrameIndex with an stack/frame pointer direct reference.
  void eliminateFrameIndex(MachineBasicBlock::iterator II,
                           int SPAdj, unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  bool hasBasePointer(const MachineFunction &MF) const;

  /// True if the stack can be realigned for the target.
  bool canRealignStack(const MachineFunction &MF) const override;

  unsigned getFrameRegister(const MachineFunction &MF) const override;
  unsigned getStackRegister() const { return StackPtr; }
  unsigned getBaseRegister() const { return BasePtr; }
  unsigned getGlobalBaseRegister() const { return GlobalBasePtr; }

  const TargetRegisterClass *intRegClass(unsigned Size) const;
};

} // end namespace llvm

#endif
