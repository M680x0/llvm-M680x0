//===-- M680x0RegisterInfo.cpp - CPU0 Register Information -== --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the CPU0 implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "m680x0-reg-info"

#include "M680x0RegisterInfo.h"

#include "M680x0.h"
#include "M680x0Subtarget.h"
#include "M680x0MachineFunction.h"

#include "MCTargetDesc/M680x0MCTargetDesc.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#define GET_REGINFO_TARGET_DESC
#include "M680x0GenRegisterInfo.inc"

using namespace llvm;

// Pin the vtable to this file.
void M680x0RegisterInfo::anchor() {}

M680x0RegisterInfo::M680x0RegisterInfo(const M680x0Subtarget &ST)
    // FIXME not sure it this the correct value, it expects RA, but x86 passes
    // IP anyway, how this works?
  : M680x0GenRegisterInfo(M680x0::PC), Subtarget(ST) {}

//===----------------------------------------------------------------------===//
// Callee Saved Registers methods
//===----------------------------------------------------------------------===//
//
const MCPhysReg *
M680x0RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_STD_SaveList;
}

const uint32_t *
M680x0RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const {
  return CSR_STD_RegMask; 
}

// pure virtual method
BitVector M680x0RegisterInfo::
getReservedRegs(const MachineFunction &MF) const {
  static const uint16_t ReservedCPURegs[] = {
    M680x0::SP, M680x0::PC
  };
  BitVector Reserved(getNumRegs());

  for (unsigned I = 0; I < array_lengthof(ReservedCPURegs); ++I)
    Reserved.set(ReservedCPURegs[I]);

  return Reserved;
}

// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void M680x0RegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                    unsigned FIOperandNum, RegScavenger *RS) const {
}

bool
M680x0RegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

bool
M680x0RegisterInfo::trackLivenessAfterRegAlloc(const MachineFunction &MF) const {
  return true;
}

// pure virtual method
unsigned M680x0RegisterInfo::
getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
  return TFI->hasFP(MF) ? (M680x0::FP) :
                          (M680x0::SP);
}

// FIXME use size
const TargetRegisterClass * M680x0RegisterInfo::
intRegClass(unsigned size) const {
  return &M680x0::DR32RegClass;
}
