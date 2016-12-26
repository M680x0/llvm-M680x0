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
    // FIXME would be nice to have tablegen level name aliasing
    StackPtr      = M680x0::SP;
    FramePtr      = M680x0::A6;
    GlobalBasePtr = M680x0::A5;
    BasePtr       = M680x0::A4;
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
  return &M680x0::XR32_TCRegClass;
}

unsigned M680x0RegisterInfo::
getMatchingMegaReg(unsigned Reg, const TargetRegisterClass *RC) const {
for (MCSuperRegIterator Super(Reg, this); Super.isValid(); ++Super)
    if (RC->contains(*Super))
      return *Super;
  return 0;
}

const TargetRegisterClass * M680x0RegisterInfo::
getMaximalPhysRegClass(unsigned reg, MVT VT) const {
  assert(isPhysicalRegister(reg) && "reg must be a physical register");

  // Pick the most sub register class of the right type that contains
  // this physreg.
  const TargetRegisterClass* BestRC = nullptr;
  for (regclass_iterator I = regclass_begin(), E = regclass_end(); I != E; ++I){
      const TargetRegisterClass* RC = *I;
      if ((VT == MVT::Other || RC->hasType(VT)) && RC->contains(reg) &&
          (!BestRC ||
           (BestRC->hasSubClass(RC) && RC->getNumRegs() > BestRC->getNumRegs())))
          BestRC = RC;
  }

  assert(BestRC && "Couldn't find the register class");
  return BestRC;
}

int M680x0RegisterInfo::
getRegisterOrder(unsigned Reg, const TargetRegisterClass &TRC) const{
  for (unsigned i = 0; i < TRC.getNumRegs(); ++i) {
    if (regsOverlap(Reg, TRC.getRegister(i))) {
      return i;
    }
  }
  return -1;
}

int M680x0RegisterInfo::
getSpillRegisterOrder(unsigned Reg) const{
  int Result = getRegisterOrder(Reg, *getRegClass(M680x0::SPILLRegClassID));
  if (Result < 0) {
    llvm_unreachable("Cannot determine spill order");
  }
  return Result;
}

BitVector M680x0RegisterInfo::
getReservedRegs(const MachineFunction &MF) const {
  const M680x0FrameLowering *TFI = getFrameLowering(MF);

  BitVector Reserved(getNumRegs());

  // Set the stack-pointer register and its aliases as reserved.
  for (MCSubRegIterator I(M680x0::SP, this, /*Self=*/true); I.isValid(); ++I)
    Reserved.set(*I);

  // Set the instruction pointer register and its aliases as reserved.
  for (MCSubRegIterator I(M680x0::PC, this, /*Self=*/true); I.isValid(); ++I)
    Reserved.set(*I);

  // Set the frame-pointer register and its aliases as reserved if needed.
  if (TFI->hasFP(MF)) {
    for (MCSubRegIterator I(FramePtr, this, /*Self=*/true); I.isValid(); ++I)
      Reserved.set(*I);
  }

  // Set the base-pointer register and its aliases as reserved if needed.
  if (hasBasePointer(MF)) {
    CallingConv::ID CC = MF.getFunction()->getCallingConv();
    const uint32_t *RegMask = getCallPreservedMask(MF, CC);
    if (MachineOperand::clobbersPhysReg(RegMask, getBaseRegister()))
      report_fatal_error(
        "Stack realignment in presence of dynamic allocas is not supported with"
        "this calling convention.");

    for (MCSubRegIterator I(getBaseRegister(), this, /*Self=*/true); I.isValid(); ++I)
      Reserved.set(*I);
  }

  // // Set the global-base-pointer register and its aliases as reserved if needed.
  // if (getGlobalBaseRegister()) {
  //   for (MCSubRegIterator I(getGlobalBaseRegister(), this, #<{(|Self=|)}>#true); I.isValid(); ++I)
  //     Reserved.set(*I);
  // }

  return Reserved;
}

void M680x0RegisterInfo::
eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                    unsigned FIOperandNum, RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const M680x0FrameLowering *TFI = getFrameLowering(MF);

  // We have either (i,An,Rn) or (i,An) EA form
  // NOTE Base contains the FI and we need to backtrace a bit to get Disp
  MachineOperand &Disp = MI.getOperand(FIOperandNum - 1);
  MachineOperand &Base = MI.getOperand(FIOperandNum);

  int Imm = (int)(Disp.getImm());
  int FIndex = (int)(Base.getIndex());

  // unsigned Opc = MI.getOpcode();
  // FIXME there is no jmp from mem yet
  // bool AfterFPPop =  Opc == M680x0::TAILJMPm || Opc == M680x0::TCRETURNmi;
  bool AfterFPPop =  false;

  unsigned BasePtr;
  if (hasBasePointer(MF))
    BasePtr = (FIndex < 0 ? FramePtr : getBaseRegister());
  else if (needsStackRealignment(MF))
    BasePtr = (FIndex < 0 ? FramePtr : StackPtr);
  else if (AfterFPPop)
    BasePtr = StackPtr;
  else
    BasePtr = (TFI->hasFP(MF) ? FramePtr : StackPtr);

  Base.ChangeToRegister(BasePtr, false);

  // Now add the frame object offset to the offset from FP.
  int FIOffset;
  unsigned IgnoredFrameReg;
  if (AfterFPPop) {
    // Tail call jmp happens after FP is popped.
    const MachineFrameInfo &MFI = MF.getFrameInfo();
    FIOffset = MFI.getObjectOffset(FIndex) - TFI->getOffsetOfLocalArea();
  } else {
    FIOffset = TFI->getFrameIndexReference(MF, FIndex, IgnoredFrameReg);
  }

  if (BasePtr == StackPtr)
    FIOffset += SPAdj;

  long long Offset = FIOffset + Imm;
  // if (Size == 16) {
  //     assert(isInt<16>(Offset) && "Cannot use disp greater 16 bit");
  // } else if (Size == 8) {
  //     assert(isInt<8>(Offset) && "Cannot use disp greater 8 bit");
  // } else {
  // }
  Disp.ChangeToImmediate(Offset);
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
  return TFI->hasFP(MF) ? FramePtr : StackPtr;
}

const TargetRegisterClass * M680x0RegisterInfo::
intRegClass(unsigned size) const {
  // if (isInt<8>(size)) {
  //     return &M680x0::DR8RegClass;
  // } else if (isInt<16>(size)) {
  //     return &M680x0::DR16RegClass;
  // }
  return &M680x0::DR32RegClass;
}
