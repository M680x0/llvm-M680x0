//===-- M680x0FrameLowering.cpp - M680x0 Frame Information --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the M680x0 implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "M680x0FrameLowering.h"

#include "M680x0InstrInfo.h"
#include "M680x0MachineFunction.h"
#include "M680x0Subtarget.h"

#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

M680x0FrameLowering::
M680x0FrameLowering(const M680x0Subtarget &STI, unsigned Alignment)
  : TargetFrameLowering(StackGrowsDown, Alignment, 0, Alignment),
      STI(STI), TII(*STI.getInstrInfo()), TRI(STI.getRegisterInfo()) {
  SlotSize = STI.getSlotSize();
  StackPtr = TRI->getStackRegister();
}

bool M680x0FrameLowering::
hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();

  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
      MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken() ||
      TRI->needsStackRealignment(MF);
}

bool M680x0FrameLowering::
hasReservedCallFrame(const MachineFunction &MF) const {
  return !MF.getFrameInfo().hasVarSizedObjects() &&
         !MF.getInfo<M680x0MachineFunctionInfo>()->getHasPushSequences();
}

bool M680x0FrameLowering::
canSimplifyCallFramePseudos(const MachineFunction &MF) const {
  return hasReservedCallFrame(MF) ||
         (hasFP(MF) && !TRI->needsStackRealignment(MF)) ||
         TRI->hasBasePointer(MF);
}

bool M680x0FrameLowering::
needsFrameIndexResolution(const MachineFunction &MF) const {
  return MF.getFrameInfo().hasStackObjects() ||
         MF.getInfo<M680x0MachineFunctionInfo>()->getHasPushSequences();
}

// NOTE: this only has a subset of the full frame index logic. In
// particular, the FI < 0 and AfterFPPop logic is handled in
// M680x0RegisterInfo::eliminateFrameIndex, but not here. Possibly
// (probably?) it should be moved into here.
int M680x0FrameLowering::
getFrameIndexReference(const MachineFunction &MF, int FI,
                       unsigned &FrameReg) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  // We can't calculate offset from frame pointer if the stack is realigned,
  // so enforce usage of stack/base pointer.  The base pointer is used when we
  // have dynamic allocas in addition to dynamic realignment.
  if (TRI->hasBasePointer(MF))
    FrameReg = TRI->getBaseRegister();
  else if (TRI->needsStackRealignment(MF))
    FrameReg = TRI->getStackRegister();
  else
    FrameReg = TRI->getFrameRegister(MF);

  // Offset will hold the offset from the stack pointer at function entry to the
  // object.
  // We need to factor in additional offsets applied during the prologue to the
  // frame, base, and stack pointer depending on which is used.
  int Offset = MFI.getObjectOffset(FI) - getOffsetOfLocalArea();
  const M680x0MachineFunctionInfo *MMFI = MF.getInfo<M680x0MachineFunctionInfo>();
  uint64_t StackSize = MFI.getStackSize();
  bool HasFP = hasFP(MF);
  int64_t FPDelta = 0;

  if (TRI->hasBasePointer(MF)) {
    assert(HasFP && "VLAs and dynamic stack realign, but no FP?!");
    if (FI < 0) {
      // Skip the saved FP.
      return Offset + SlotSize + FPDelta;
    } else {
      assert((-(Offset + StackSize)) % MFI.getObjectAlignment(FI) == 0);
      return Offset + StackSize;
    }
  } else if (TRI->needsStackRealignment(MF)) {
    if (FI < 0) {
      // Skip the saved FP.
      return Offset + SlotSize + FPDelta;
    } else {
      assert((-(Offset + StackSize)) % MFI.getObjectAlignment(FI) == 0);
      return Offset + StackSize;
    }
    // FIXME: Support tail calls
  } else {
    if (!HasFP)
      return Offset + StackSize;

    // Skip the saved FP.
    Offset += SlotSize;

    // Skip the RETADDR move area
    int TailCallReturnAddrDelta = MMFI->getTCReturnAddrDelta();
    if (TailCallReturnAddrDelta < 0)
      Offset -= TailCallReturnAddrDelta;
  }

  return Offset + FPDelta;
}

static unsigned getSUBriOpcode(int64_t Imm) {
  return M680x0::SUB32ri;
}

static unsigned getADDriOpcode(int64_t Imm) {
  return M680x0::ADD32ri;
}

static unsigned getSUBrrOpcode() {
  return M680x0::SUB32rr;
}

static unsigned getADDrrOpcode() {
  return M680x0::ADD32rr;
}

static unsigned getANDriOpcode(int64_t Imm) {
  return M680x0::AND32di;
}

static unsigned getLEArOpcode() {
  return M680x0::LEA32r_ARID;
}

/// findDeadCallerSavedReg - Return a caller-saved register that isn't live
/// when it reaches the "return" instruction. We can then pop a stack object
/// to this register without worry about clobbering it.
static unsigned findDeadCallerSavedReg(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator &MBBI,
                                       const M680x0RegisterInfo *TRI) {
  const MachineFunction *MF = MBB.getParent();
  const Function *F = MF->getFunction();
  if (!F || MF->getMMI().callsEHReturn())
    return 0;

  const TargetRegisterClass &AvailableRegs = *TRI->getRegsForTailCall(*MF);

  if (MBBI == MBB.end())
    return 0;

  switch (MBBI->getOpcode()) {
  default: return 0;
  case TargetOpcode::PATCHABLE_RET:
  case M680x0::RET: {
    SmallSet<uint16_t, 8> Uses;
    for (unsigned i = 0, e = MBBI->getNumOperands(); i != e; ++i) {
      MachineOperand &MO = MBBI->getOperand(i);
      if (!MO.isReg() || MO.isDef())
        continue;
      unsigned Reg = MO.getReg();
      if (!Reg)
        continue;
      for (MCRegAliasIterator AI(Reg, TRI, true); AI.isValid(); ++AI)
        Uses.insert(*AI);
    }

    for (auto CS : AvailableRegs)
      if (!Uses.count(CS))
        return CS;
  }
  }

  return 0;
}

static bool isRegLiveIn(MachineBasicBlock &MBB, unsigned Reg) {
  for (MachineBasicBlock::RegisterMaskPair RegMask : MBB.liveins()) {
    if (RegMask.PhysReg == Reg)
      return true;
  }

  return false;
}

/// addRegIndirectWithDisp - This function is used to add a memory reference of the form
/// [Reg + Offset], i.e., one with no scale or index, but with a
/// displacement. An example is: (4,D0).
/// TODO move it to some helper file
static const MachineInstrBuilder &
addRegIndirectWithDisp(const MachineInstrBuilder &MIB,
             unsigned Reg, bool isKill, int Offset) {
  return MIB.addImm(Offset).addReg(Reg, getKillRegState(isKill));
}

uint64_t M680x0FrameLowering::
calculateMaxStackAlign(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  uint64_t MaxAlign = MFI.getMaxAlignment(); // Desired stack alignment.
  unsigned StackAlign = getStackAlignment(); // ABI alignment
  if (MF.getFunction()->hasFnAttribute("stackrealign")) {
    if (MFI.hasCalls())
      MaxAlign = (StackAlign > MaxAlign) ? StackAlign : MaxAlign;
    else if (MaxAlign < SlotSize)
      MaxAlign = SlotSize;
  }
  return MaxAlign;
}

void M680x0FrameLowering::
BuildStackAlignAND(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                   const DebugLoc &DL, unsigned Reg, uint64_t MaxAlign) const {
  uint64_t Val = -MaxAlign;
  unsigned AndOp = getANDriOpcode(Val);
  MachineInstr *MI = BuildMI(MBB, MBBI, DL, TII.get(AndOp), Reg)
                         .addReg(Reg)
                         .addImm(Val)
                         .setMIFlag(MachineInstr::FrameSetup);

  // The CCR implicit def is dead.
  // FIXME CCR is not yet present
  MI->getOperand(3).setIsDead();
}

/// emitSPUpdate - Emit a series of instructions to increment / decrement the
/// stack pointer by a constant value.
void M680x0FrameLowering::
emitSPUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
             int64_t NumBytes, bool InEpilogue) const {
  bool isSub = NumBytes < 0;
  uint64_t Offset = isSub ? -NumBytes : NumBytes;

  uint64_t Chunk = (1LL << 31) - 1;
  DebugLoc DL = MBB.findDebugLoc(MBBI);

  while (Offset) {
    if (Offset > Chunk) {
      // Rather than emit a long series of instructions for large offsets,
      // load the offset into a register and do one sub/add
      unsigned Reg = 0;

      if (isSub && !isRegLiveIn(MBB, M680x0::D0))
        Reg = (unsigned)(M680x0::D0);
      else
        Reg = findDeadCallerSavedReg(MBB, MBBI, TRI);

      if (Reg) {
        unsigned Opc = M680x0::MOV32ri;
        BuildMI(MBB, MBBI, DL, TII.get(Opc), Reg)
          .addImm(Offset);
        Opc = isSub
          ? getSUBrrOpcode()
          : getADDrrOpcode();
        MachineInstr *MI = BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr)
          .addReg(StackPtr)
          .addReg(Reg);
        // ??? still no CCR
        MI->getOperand(3).setIsDead(); // The CCR implicit def is dead.
        Offset = 0;
        continue;
      }
    }

    uint64_t ThisVal = std::min(Offset, Chunk);

    MachineInstrBuilder MI = BuildStackAdjustment(
        MBB, MBBI, DL, isSub ? -ThisVal : ThisVal, InEpilogue);
    if (isSub)
      MI.setMIFlag(MachineInstr::FrameSetup);
    else
      MI.setMIFlag(MachineInstr::FrameDestroy);

    Offset -= ThisVal;
  }
}

int M680x0FrameLowering::
mergeSPUpdates(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
               bool doMergeWithPrevious) const {
  if ((doMergeWithPrevious && MBBI == MBB.begin()) ||
      (!doMergeWithPrevious && MBBI == MBB.end()))
    return 0;

  MachineBasicBlock::iterator PI = doMergeWithPrevious ? std::prev(MBBI) : MBBI;
  MachineBasicBlock::iterator NI = doMergeWithPrevious ? nullptr
                                                       : std::next(MBBI);
  unsigned Opc = PI->getOpcode();
  int Offset = 0;

  if (!doMergeWithPrevious && NI != MBB.end() &&
      NI->getOpcode() == TargetOpcode::CFI_INSTRUCTION) {
    // Don't merge with the next instruction if it has CFI.
    return Offset;
  }

  if (Opc == M680x0::ADD32ri && PI->getOperand(0).getReg() == StackPtr) {
    assert(PI->getOperand(1).getReg() == StackPtr);
    Offset += PI->getOperand(2).getImm();
    MBB.erase(PI);
    if (!doMergeWithPrevious) MBBI = NI;
  } else if (Opc == M680x0::LEA32r_ARID &&
             PI->getOperand(0).getReg() == StackPtr &&
             PI->getOperand(2).getReg() == StackPtr) {
    Offset += PI->getOperand(1).getImm();
    MBB.erase(PI);
    if (!doMergeWithPrevious) MBBI = NI;
  } else if (Opc == M680x0::SUB32ri && PI->getOperand(0).getReg() == StackPtr) {
    assert(PI->getOperand(1).getReg() == StackPtr);
    Offset -= PI->getOperand(2).getImm();
    MBB.erase(PI);
    if (!doMergeWithPrevious) MBBI = NI;
  }

  return Offset;
}

MachineInstrBuilder M680x0FrameLowering::
BuildStackAdjustment(MachineBasicBlock &MBB,
                     MachineBasicBlock::iterator MBBI,
                     const DebugLoc &DL, int64_t Offset,
                     bool InEpilogue) const {
  assert(Offset != 0 && "zero offset stack adjustment requested");

  // ??? in the original code for M680x0 Atom uses lea to adjust stack as an
  // optimization, can be be this applied for M680x0?

  bool IsSub = Offset < 0;
  uint64_t AbsOffset = IsSub ? -Offset : Offset;
  unsigned Opc = IsSub ? getSUBriOpcode(AbsOffset)
                       : getADDriOpcode(AbsOffset);

  MachineInstrBuilder MI = BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr)
    .addReg(StackPtr)
    .addImm(AbsOffset);
  // FIXME ATM there is no CCR in these inst
  MI->getOperand(3).setIsDead(); // The CCR implicit def is dead.
  return MI;
}

void M680x0FrameLowering::
BuildCFI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
         const DebugLoc &DL, const MCCFIInstruction &CFIInst) const {
  MachineFunction &MF = *MBB.getParent();
  unsigned CFIIndex = MF.getMMI().addFrameInst(CFIInst);
  BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
      .addCFIIndex(CFIIndex);
}

void M680x0FrameLowering::
emitCalleeSavedFrameMoves(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MBBI,
                          const DebugLoc &DL) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  MachineModuleInfo &MMI = MF.getMMI();
  const MCRegisterInfo *MRI = MMI.getContext().getRegisterInfo();

  // Add callee saved registers to move list.
  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();
  if (CSI.empty()) return;

  // Calculate offsets.
  for (std::vector<CalleeSavedInfo>::const_iterator
         I = CSI.begin(), E = CSI.end(); I != E; ++I) {
    int64_t Offset = MFI.getObjectOffset(I->getFrameIdx());
    unsigned Reg = I->getReg();

    unsigned DwarfReg = MRI->getDwarfRegNum(Reg, true);
    BuildCFI(MBB, MBBI, DL,
             MCCFIInstruction::createOffset(nullptr, DwarfReg, Offset));
  }
}

void M680x0FrameLowering::
emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  assert(&STI == &MF.getSubtarget<M680x0Subtarget>() &&
         "MF used frame lowering for wrong subtarget");

  MachineBasicBlock::iterator MBBI = MBB.begin();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const Function *Fn = MF.getFunction();
  MachineModuleInfo &MMI = MF.getMMI();
  M680x0MachineFunctionInfo *MMFI = MF.getInfo<M680x0MachineFunctionInfo>();
  uint64_t MaxAlign = calculateMaxStackAlign(MF);// Desired stack alignment.
  uint64_t StackSize = MFI.getStackSize();       // Number of bytes to allocate.
  EHPersonality Personality = EHPersonality::Unknown;
  if (Fn->hasPersonalityFn())
    Personality = classifyEHPersonality(Fn->getPersonalityFn());
  bool HasFP = hasFP(MF);
  bool NeedsDwarfCFI = MMI.hasDebugInfo() || Fn->needsUnwindTableEntry();
  unsigned FramePtr = TRI->getFrameRegister(MF);
  const unsigned MachineFramePtr = FramePtr;
  unsigned BasePtr = TRI->getBaseRegister();

  // Debug location must be unknown since the first debug location is used
  // to determine the end of the prologue.
  DebugLoc DL;

  // Add RETADDR move area to callee saved frame size.
  int TailCallReturnAddrDelta = MMFI->getTCReturnAddrDelta();

  if (TailCallReturnAddrDelta < 0) {
    MMFI->setCalleeSavedFrameSize(
      MMFI->getCalleeSavedFrameSize() - TailCallReturnAddrDelta);
  }

  // Insert stack pointer adjustment for later moving of return addr.  Only
  // applies to tail call optimized functions where the callee argument stack
  // size is bigger than the callers.
  if (TailCallReturnAddrDelta < 0) {
    BuildStackAdjustment(MBB, MBBI, DL, TailCallReturnAddrDelta,
                         /*InEpilogue=*/false)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  // Mapping for machine moves:
  //
  //   DST: VirtualFP AND
  //        SRC: VirtualFP              => DW_CFA_def_cfa_offset
  //        ELSE                        => DW_CFA_def_cfa
  //
  //   SRC: VirtualFP AND
  //        DST: Register               => DW_CFA_def_cfa_register
  //
  //   ELSE
  //        OFFSET < 0                  => DW_CFA_offset_extended_sf
  //        REG < 64                    => DW_CFA_offset + Reg
  //        ELSE                        => DW_CFA_offset_extended

  uint64_t NumBytes = 0;
  int stackGrowth = -SlotSize;

  if (HasFP) {
    // Calculate required stack adjustment.
    uint64_t FrameSize = StackSize - SlotSize;
    // If required, include space for extra hidden slot for stashing base pointer.
    if (MMFI->getRestoreBasePointer())
      FrameSize += SlotSize;

    NumBytes = FrameSize - MMFI->getCalleeSavedFrameSize();

    // Callee-saved registers are pushed on stack before the stack is realigned.
    if (TRI->needsStackRealignment(MF))
      NumBytes = alignTo(NumBytes, MaxAlign);

    // Get the offset of the stack slot for the FP register, which is
    // guaranteed to be the last slot by processFunctionBeforeFrameFinalized.
    // Update the frame offset adjustment.
    MFI.setOffsetAdjustment(-NumBytes);

    // Save FP into the appropriate stack slot.
    BuildMI(MBB, MBBI, DL, TII.get(M680x0::PUSH32r))
      .addReg(MachineFramePtr, RegState::Kill)
      .setMIFlag(MachineInstr::FrameSetup);

    if (NeedsDwarfCFI) {
      // Mark the place where FP was saved.
      // Define the current CFA rule to use the provided offset.
      assert(StackSize);
      BuildCFI(MBB, MBBI, DL,
               MCCFIInstruction::createDefCfaOffset(nullptr, 2 * stackGrowth));

      // Change the rule for the FramePtr to be an "offset" rule.
      unsigned DwarfFramePtr = TRI->getDwarfRegNum(MachineFramePtr, true);
      BuildCFI(MBB, MBBI, DL, MCCFIInstruction::createOffset(
                                  nullptr, DwarfFramePtr, 2 * stackGrowth));
    }

    // Update FP with the new base value.
    BuildMI(MBB, MBBI, DL, TII.get(M680x0::MOV32aa), FramePtr)
        .addReg(StackPtr)
        .setMIFlag(MachineInstr::FrameSetup);

    if (NeedsDwarfCFI) {
      // Mark effective beginning of when frame pointer becomes valid.
      // Define the current CFA to use the FP register.
      unsigned DwarfFramePtr = TRI->getDwarfRegNum(MachineFramePtr, true);
      BuildCFI(MBB, MBBI, DL, MCCFIInstruction::createDefCfaRegister(
                                  nullptr, DwarfFramePtr));
    }

    // Mark the FramePtr as live-in in every block. Don't do this again for
    // funclet prologues.
    for (MachineBasicBlock &EveryMBB : MF)
      EveryMBB.addLiveIn(MachineFramePtr);
  } else {
    NumBytes = StackSize - MMFI->getCalleeSavedFrameSize();
  }

  // Skip the callee-saved push instructions.
  bool PushedRegs = false;
  int StackOffset = 2 * stackGrowth;

  while (MBBI != MBB.end() &&
         MBBI->getFlag(MachineInstr::FrameSetup) &&
         MBBI->getOpcode() == M680x0::PUSH32r) {
    PushedRegs = true;
    ++MBBI;

    if (!HasFP && NeedsDwarfCFI) {
      // Mark callee-saved push instruction.
      // Define the current CFA rule to use the provided offset.
      assert(StackSize);
      BuildCFI(MBB, MBBI, DL,
               MCCFIInstruction::createDefCfaOffset(nullptr, StackOffset));
      StackOffset += stackGrowth;
    }
  }

  // Realign stack after we pushed callee-saved registers (so that we'll be
  // able to calculate their offsets from the frame pointer).
  if (TRI->needsStackRealignment(MF)) {
    assert(HasFP && "There should be a frame pointer if stack is realigned.");
    BuildStackAlignAND(MBB, MBBI, DL, StackPtr, MaxAlign);
  }

  // If there is an SUB32ri of SP immediately before this instruction, merge
  // the two. This can be the case when tail call elimination is enabled and
  // the callee has more arguments then the caller.
  NumBytes -= mergeSPUpdates(MBB, MBBI, true);

  // Adjust stack pointer: ESP -= numbytes.
  emitSPUpdate(MBB, MBBI, -(int64_t)NumBytes, /*InEpilogue=*/false);

  unsigned SPOrEstablisher = StackPtr;

  // If we need a base pointer, set it up here. It's whatever the value
  // of the stack pointer is at this point. Any variable size objects
  // will be allocated after this, so we can still use the base pointer
  // to reference locals.
  if (TRI->hasBasePointer(MF)) {
    // Update the base pointer with the current stack pointer.
    BuildMI(MBB, MBBI, DL, TII.get(M680x0::MOV32aa), BasePtr)
      .addReg(SPOrEstablisher)
      .setMIFlag(MachineInstr::FrameSetup);
    if (MMFI->getRestoreBasePointer()) {
      // Stash value of base pointer.  Saving SP instead of FP shortens
      // dependence chain. Used by SjLj EH.
      unsigned Opm =  M680x0::MOV32ja;
      addRegIndirectWithDisp(BuildMI(MBB, MBBI, DL, TII.get(Opm)),
                   FramePtr, true, MMFI->getRestoreBasePointerOffset())
        .addReg(SPOrEstablisher)
        .setMIFlag(MachineInstr::FrameSetup);
    }
  }

  if (((!HasFP && NumBytes) || PushedRegs) && NeedsDwarfCFI) {
    // Mark end of stack pointer adjustment.
    if (!HasFP && NumBytes) {
      // Define the current CFA rule to use the provided offset.
      assert(StackSize);
      BuildCFI(MBB, MBBI, DL, MCCFIInstruction::createDefCfaOffset(
                                  nullptr, -StackSize + stackGrowth));
    }

    // Emit DWARF info specifying the offsets of the callee-saved registers.
    if (PushedRegs)
      emitCalleeSavedFrameMoves(MBB, MBBI, DL);
  }

  // TODO interrupts...
  // M680x0 Interrupt handling function cannot assume anything about the direction
  // flag (DF in EFLAGS register). Clear this flag by creating "cld" instruction
  // in each prologue of interrupt handler function.
  //
  // FIXME: Create "cld" instruction only in these cases:
  // 1. The interrupt handling function uses any of the "rep" instructions.
  // 2. Interrupt handling function calls another function.
  //
  // if (Fn->getCallingConv() == CallingConv::M680x0_INTR)
  //   BuildMI(MBB, MBBI, DL, TII.get(M680x0::CLD))
  //       .setMIFlag(MachineInstr::FrameSetup);
}

static bool isTailCallOpcode(unsigned Opc) {
    return Opc == M680x0::TCRETURNj || Opc == M680x0::TCRETURNq;
}

void M680x0FrameLowering::
emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  M680x0MachineFunctionInfo *MMFI = MF.getInfo<M680x0MachineFunctionInfo>();
  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
  Optional<unsigned> RetOpcode;
  if (MBBI != MBB.end())
    RetOpcode = MBBI->getOpcode();
  DebugLoc DL;
  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();
  unsigned FramePtr = TRI->getFrameRegister(MF);
  unsigned MachineFramePtr = FramePtr;

  // Get the number of bytes to allocate from the FrameInfo.
  uint64_t StackSize = MFI.getStackSize();
  uint64_t MaxAlign = calculateMaxStackAlign(MF);
  unsigned CSSize = MMFI->getCalleeSavedFrameSize();
  uint64_t NumBytes = 0;

  if (hasFP(MF)) {
    // Calculate required stack adjustment.
    uint64_t FrameSize = StackSize - SlotSize;
    NumBytes = FrameSize - CSSize;

    // Callee-saved registers were pushed on stack before the stack was
    // realigned.
    if (TRI->needsStackRealignment(MF))
      NumBytes = alignTo(FrameSize, MaxAlign);

    // Pop FP.
    BuildMI(MBB, MBBI, DL, TII.get(M680x0::POP32r), MachineFramePtr)
        .setMIFlag(MachineInstr::FrameDestroy);
  } else {
    NumBytes = StackSize - CSSize;
  }

  // Skip the callee-saved pop instructions.
  while (MBBI != MBB.begin()) {
    MachineBasicBlock::iterator PI = std::prev(MBBI);
    unsigned Opc = PI->getOpcode();

    if ((Opc != M680x0::POP32r || !PI->getFlag(MachineInstr::FrameDestroy)) &&
        Opc != M680x0::DBG_VALUE && !PI->isTerminator())
      break;

    --MBBI;
  }
  MachineBasicBlock::iterator FirstCSPop = MBBI;

  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();

  // If there is an ADD32ri or SUB32ri of SP immediately before this
  // instruction, merge the two instructions.
  if (NumBytes || MFI.hasVarSizedObjects())
    NumBytes += mergeSPUpdates(MBB, MBBI, true);

  // If dynamic alloca is used, then reset SP to point to the last callee-saved
  // slot before popping them off! Same applies for the case, when stack was
  // realigned. Don't do this if this was a funclet epilogue, since the funclets
  // will not do realignment or dynamic stack allocation.
  if ((TRI->needsStackRealignment(MF) || MFI.hasVarSizedObjects())) {
    if (TRI->needsStackRealignment(MF))
      MBBI = FirstCSPop;
    uint64_t LEAAmount = -CSSize;

    // 'move %FramePtr, SP' will not be recognized as an epilogue sequence.
    // However, we may use this sequence if we have a frame pointer because the
    // effects of the prologue can safely be undone.
    if (LEAAmount != 0) {
      unsigned Opc = getLEArOpcode();
      addRegIndirectWithDisp(BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr),
                             FramePtr, false, LEAAmount);
      --MBBI;
    } else {
      unsigned Opc = (M680x0::MOV32rr);
      BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr)
        .addReg(FramePtr);
      --MBBI;
    }
  } else if (NumBytes) {
    // Adjust stack pointer back: SP += numbytes.
    emitSPUpdate(MBB, MBBI, NumBytes, /*InEpilogue=*/true);
    --MBBI;
  }

  if (!RetOpcode || !isTailCallOpcode(*RetOpcode)) {
    // Add the return addr area delta back since we are not tail calling.
    int Offset = -1 * MMFI->getTCReturnAddrDelta();
    assert(Offset >= 0 && "TCDelta should never be positive");
    if (Offset) {
      MBBI = MBB.getFirstTerminator();

      // Check for possible merge with preceding ADD instruction.
      Offset += mergeSPUpdates(MBB, MBBI, true);
      emitSPUpdate(MBB, MBBI, Offset, /*InEpilogue=*/true);
    }
  }
}
