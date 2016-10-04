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

class M680x0InstrInfo : public M680x0GenInstrInfo {
  virtual void anchor();
protected:
  const M680x0Subtarget &Subtarget;
  const M680x0RegisterInfo RI;
public:
  explicit M680x0InstrInfo(const M680x0Subtarget &STI);

  static const M680x0InstrInfo *create(M680x0Subtarget &STI);

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

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  const M680x0RegisterInfo &getRegisterInfo() const { return RI; };

private:
  bool ExpandMOVSZX_RR(MachineInstrBuilder &MIB, bool isSigned,
                      MVT MVTDst, MVT MVTSrc) const;

  bool ExpandMOVSZX_RM(MachineInstrBuilder &MIB, bool isSigned,
                     const MCInstrDesc &Desc,
                     MVT MVTDst, MVT MVTSrc) const;

  bool ExpandMOVX_RR(MachineInstrBuilder &MIB, const MCInstrDesc &Desc,
                     MVT MVTDst, MVT MVTSrc) const;
};

} // namespace llvm

#endif
