//===-- M680x0FrameLowering.h - Define frame lowering for M680x0 --- C++ --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the M680x0 declaration of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_M680X0_M680X0FRAMELOWERING_H
#define LLVM_LIB_TARGET_M680X0_M680X0FRAMELOWERING_H

#include "M680x0.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {
class MachineInstrBuilder;
class MCCFIInstruction;
class M680x0Subtarget;
class M680x0RegisterInfo;

class M680x0FrameLowering : public TargetFrameLowering {
  // Cached subtarget predicates.
  const M680x0Subtarget    &STI;
  const TargetInstrInfo    &TII;
  const M680x0RegisterInfo *TRI;

  /// Stack slot size in bytes.
  unsigned SlotSize;

  unsigned StackPtr;

  // If we're forcing a stack realignment we can't rely on just the frame
  // info, we need to know the ABI stack alignment as well in case we
  // have a call out.  Otherwise just make sure we have some alignment - we'll
  // go with the minimum SlotSize.
  uint64_t calculateMaxStackAlign(const MachineFunction &MF) const;

  /// Adjusts the stack pointer using LEA, SUB, or ADD.
  MachineInstrBuilder BuildStackAdjustment(MachineBasicBlock &MBB,
                                           MachineBasicBlock::iterator MBBI,
                                           const DebugLoc &DL, int64_t Offset,
                                           bool InEpilogue) const;

  /// Aligns the stack pointer by ANDing it with -MaxAlign.
  void BuildStackAlignAND(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MBBI, const DebugLoc &DL,
                          unsigned Reg, uint64_t MaxAlign) const;

  /// Wraps up getting a CFI index and building a MachineInstr for it.
  void BuildCFI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                const DebugLoc &DL, const MCCFIInstruction &CFIInst) const;

  void emitCalleeSavedFrameMoves(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 const DebugLoc &DL) const;

  unsigned getPSPSlotOffsetFromSP(const MachineFunction &MF) const;

public:
  explicit M680x0FrameLowering(const M680x0Subtarget &sti, unsigned Alignment);

  static const M680x0FrameLowering *create(const M680x0Subtarget &ST);

  /// This method is called during prolog/epilog code insertion to eliminate
  /// call frame setup and destroy pseudo instructions (but only if the Target
  /// is using them).  It is responsible for eliminating these instructions,
  /// replacing them with concrete instructions.  This method need only be
  /// implemented if using call frame setup/destroy pseudo instructions.
  /// Returns an iterator pointing to the instruction after the replaced one.
  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MI) const override;

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  /// This method determines which of the registers reported by
  /// TargetRegisterInfo::getCalleeSavedRegs() should actually get saved.
  /// The default implementation checks populates the \p SavedRegs bitset with
  /// all registers which are modified in the function, targets may override
  /// this function to save additional registers.
  /// This method also sets up the register scavenger ensuring there is a free
  /// register or a frameindex available.
  void determineCalleeSaves(MachineFunction &MF, BitVector &SavedRegs,
                            RegScavenger *RS = nullptr) const override;

  /// assignCalleeSavedSpillSlots - Allows target to override spill slot
  /// assignment logic.  If implemented, assignCalleeSavedSpillSlots() should
  /// assign frame slots to all CSI entries and return true.  If this method
  /// returns false, spill slots will be assigned using generic implementation.
  /// assignCalleeSavedSpillSlots() may add, delete or rearrange elements of CSI.
  bool assignCalleeSavedSpillSlots(MachineFunction &MF,
                              const TargetRegisterInfo *TRI,
                              std::vector<CalleeSavedInfo> &CSI) const override;

  /// spillCalleeSavedRegisters - Issues instruction(s) to spill all callee
  /// saved registers and returns true if it isn't possible / profitable to do
  /// so by issuing a series of store instructions via
  /// storeRegToStackSlot(). Returns false otherwise.
  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const override;

  /// restoreCalleeSavedRegisters - Issues instruction(s) to restore all callee
  /// saved registers and returns true if it isn't possible / profitable to do
  /// so by issuing a series of load instructions via loadRegToStackSlot().
  /// Returns false otherwise.
  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MI,
                                  std::vector<CalleeSavedInfo> &CSI,
                                  const TargetRegisterInfo *TRI) const override;

  // hasFP - Return true if the specified function should have a dedicated frame
  // pointer register.  This is true if the function has variable sized allocas,
  // if it needs dynamic stack realignment, if frame pointer elimination is
  // disabled, or if the frame address is taken.
  bool hasFP(const MachineFunction &MF) const override;

  /// hasReservedCallFrame - Under normal circumstances, when a frame pointer is
  /// not required, we reserve argument space for call sites in the function
  /// immediately on entry to the current function. This eliminates the need for
  /// add/sub sp brackets around call sites. Returns true if the call frame is
  /// included as part of the stack frame.
  bool hasReservedCallFrame(const MachineFunction &MF) const override;

  /// canSimplifyCallFramePseudos - If there is a reserved call frame, the
  /// call frame pseudos can be simplified.  Having a FP, as in the default
  /// implementation, is not sufficient here since we can't always use it.
  /// Use a more nuanced condition.
  bool canSimplifyCallFramePseudos(const MachineFunction &MF) const override;

  // needsFrameIndexResolution - Do we need to perform FI resolution for
  // this function. Normally, this is required only when the function
  // has any stack objects. However, FI resolution actually has another job,
  // not apparent from the title - it resolves callframe setup/destroy
  // that were not simplified earlier.
  // So, this is required for M680x0 functions that have push sequences even
  // when there are no stack objects.
  bool needsFrameIndexResolution(const MachineFunction &MF) const override;

  /// getFrameIndexReference - This method should return the base register
  /// and offset used to reference a frame index location. The offset is
  /// returned directly, and the base register is returned via FrameReg.
  int getFrameIndexReference(const MachineFunction &MF, int FI,
                             unsigned &FrameReg) const override;

  /// Check the instruction before/after the passed instruction. If
  /// it is an ADD/SUB/LEA instruction it is deleted argument and the
  /// stack adjustment is returned as a positive value for ADD/LEA and
  /// a negative for SUB.
  int mergeSPUpdates(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
                     bool doMergeWithPrevious) const;

  /// Emit a series of instructions to increment / decrement the stack
  /// pointer by a constant value.
  void emitSPUpdate(MachineBasicBlock &MBB, MachineBasicBlock::iterator &MBBI,
                    int64_t NumBytes, bool InEpilogue) const;

  /// TODO
  /// Order the symbols in the local stack.
  /// We want to place the local stack objects in some sort of sensible order.
  /// The heuristic we use is to try and pack them according to static number
  /// of uses and size in order to minimize code size.
  // void orderFrameObjects(const MachineFunction &MF,
  //                        SmallVectorImpl<int> &ObjectsToAllocate) const override;
};
} // End llvm namespace

#endif
