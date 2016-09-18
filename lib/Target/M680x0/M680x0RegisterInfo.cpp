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
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#define GET_REGINFO_TARGET_DESC
#include "M680x0GenRegisterInfo.inc"

using namespace llvm;

static cl::opt<bool>
EnableBasePointer("m680x0-use-base-pointer", cl::Hidden, cl::init(true),
          cl::desc("Enable use of a base pointer for complex stack frames"));

// Pin the vtable to this file.
void M680x0RegisterInfo::anchor() {}

M680x0RegisterInfo::M680x0RegisterInfo(const M680x0Subtarget &ST)
    // FIXME not sure it this the correct value, it expects RA, but M680x0 passes
    // IP anyway, how this works?
  : M680x0GenRegisterInfo(M680x0::PC), Subtarget(ST) {
    StackPtr = M680x0::SP;
    FramePtr = M680x0::FP;
    BasePtr  = M680x0::BP;
  }

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

const TargetRegisterClass * M680x0RegisterInfo::
getRegsForTailCall(const MachineFunction &MF) const {
  const Function *F = MF.getFunction();
  return &M680x0::R32_TCRegClass;
}

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

// FrameIndex represent objects inside a abstract stack. We must replace
// FrameIndex with an stack/frame pointer direct reference.
void M680x0RegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                    unsigned FIOperandNum, RegScavenger *RS) const {
}

bool M680x0RegisterInfo::
requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

bool M680x0RegisterInfo::
trackLivenessAfterRegAlloc(const MachineFunction &MF) const {
  return true;
}

static bool CantUseSP(const MachineFrameInfo &MFI) {
  return MFI.hasVarSizedObjects() || MFI.hasOpaqueSPAdjustment();
}

bool M680x0RegisterInfo::
hasBasePointer(const MachineFunction &MF) const {
   const MachineFrameInfo &MFI = MF.getFrameInfo();

   if (!EnableBasePointer)
     return false;

   // When we need stack realignment, we can't address the stack from the frame
   // pointer.  When we have dynamic allocas or stack-adjusting inline asm, we
   // can't address variables from the stack pointer.  MS inline asm can
   // reference locals while also adjusting the stack pointer.  When we can't
   // use both the SP and the FP, we need a separate base pointer register.
   bool CantUseFP = needsStackRealignment(MF);
   return CantUseFP && CantUseSP(MFI);
}

bool M680x0RegisterInfo::
canRealignStack(const MachineFunction &MF) const {
  if (!TargetRegisterInfo::canRealignStack(MF))
    return false;

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const MachineRegisterInfo *MRI = &MF.getRegInfo();

  // Stack realignment requires a frame pointer.  If we already started
  // register allocation with frame pointer elimination, it is too late now.
  if (!MRI->canReserveReg(FramePtr))
    return false;

  // If a base pointer is necessary. Check that it isn't too late to reserve it.
  if (CantUseSP(MFI))
    return MRI->canReserveReg(BasePtr);

  return true;
}

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
