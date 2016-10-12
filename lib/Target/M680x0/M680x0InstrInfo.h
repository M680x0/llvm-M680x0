//===-- M680x0InstrInfo.h - M680x0 Instruction Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the M680x0 implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_M680X0_M680X0INSTRINFO_H
#define LLVM_LIB_TARGET_M680X0_M680X0INSTRINFO_H

#include "MCTargetDesc/M680x0BaseInfo.h"
#include "M680x0.h"
#include "M680x0RegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "M680x0GenInstrInfo.inc"

namespace llvm {

class M680x0Subtarget;

/// isGlobalStubReference - Return true if the specified TargetFlag operand is
/// a reference to a stub for a global, not the global itself.
inline static bool isGlobalStubReference(unsigned char TargetFlag) {
  switch (TargetFlag) {
  case M680x0II::MO_GOTPCREL:  // rip-relative GOT reference.
  case M680x0II::MO_GOT:       // normal GOT reference.
    return true;
  default:
    return false;
  }
}

/// isGlobalRelativeToPICBase - Return true if the specified global value
/// reference is relative to a 32-bit PIC base (M680x0ISD::GlobalBaseReg). If this
/// is true, the addressing mode has the PIC base register added in (e.g. EBX).
inline static bool isGlobalRelativeToPICBase(unsigned char TargetFlag) {
  switch (TargetFlag) {
  case M680x0II::MO_GOTOFF:          // isPICStyleGOT: local global.
  case M680x0II::MO_GOT:             // isPICStyleGOT: other global.
  case M680x0II::MO_PIC_BASE_OFFSET: // Darwin local global.
    return true;
  default:
    return false;
  }
}

class M680x0InstrInfo : public M680x0GenInstrInfo {
  virtual void anchor();
protected:
  const M680x0Subtarget &Subtarget;
  const M680x0RegisterInfo RI;
public:
  explicit M680x0InstrInfo(const M680x0Subtarget &STI);

  static const M680x0InstrInfo *create(M680x0Subtarget &STI);

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  const M680x0RegisterInfo &getRegisterInfo() const { return RI; };

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, unsigned DestReg, unsigned SrcReg,
                   bool KillSrc) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI,
                           unsigned SrcReg, bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const override;

  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI,
                            unsigned DestReg, int FrameIndex,
                            const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const override;

  bool expandPostRAPseudo(MachineInstr &MI) const override;

private:
  // Add appropriate SExt nodes
  void AddSExt(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, DebugLoc DL,
               unsigned Reg, MVT From, MVT To) const;

  // Move from register and extend
  bool ExpandMOVSZX_RR(MachineInstrBuilder &MIB, bool isSigned,
                      MVT MVTDst, MVT MVTSrc) const;

  // Move from memory and extend
  bool ExpandMOVSZX_RM(MachineInstrBuilder &MIB, bool isSigned,
                     const MCInstrDesc &Desc,
                     MVT MVTDst, MVT MVTSrc) const;

  // Move across register classes without extension
  bool ExpandMOVX_RR(MachineInstrBuilder &MIB, const MCInstrDesc &Desc,
                     MVT MVTDst, MVT MVTSrc) const;
};

} // namespace llvm

#endif
