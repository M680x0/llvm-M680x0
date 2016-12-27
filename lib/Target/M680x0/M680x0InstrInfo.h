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

namespace M680x0 {

// These MUST be kept in sync with codes definitions in M680x0InstrInfo.td
enum CondCode {
  COND_T  = 0,  // True
  COND_F  = 1,  // False
  COND_HI = 2,  // High
  COND_LS = 3,  // Less or Same
  COND_CC = 4,  // Carry Clear
  COND_CS = 5,  // Carry Set
  COND_NE = 6,  // Not Equal
  COND_EQ = 7,  // Equal
  COND_VC = 8,  // Overflow Clear
  COND_VS = 9,  // Overflow Set
  COND_PL = 10, // Plus
  COND_MI = 11, // Minus
  COND_GE = 12, // Greater or Equal
  COND_LT = 13, // Less Than
  COND_GT = 14, // Greater Than
  COND_LE = 15, // Less or Equal
  LAST_VALID_COND = COND_LE,
  COND_INVALID
};

static inline M680x0::CondCode
GetOppositeBranchCondition(M680x0::CondCode CC) {
  switch (CC) {
  default: llvm_unreachable("Illegal condition code!");
  case M680x0::COND_T:  return M680x0::COND_F;
  case M680x0::COND_F:  return M680x0::COND_T;
  case M680x0::COND_HI: return M680x0::COND_LS;
  case M680x0::COND_LS: return M680x0::COND_HI;
  case M680x0::COND_CC: return M680x0::COND_CS;
  case M680x0::COND_CS: return M680x0::COND_CC;
  case M680x0::COND_NE: return M680x0::COND_EQ;
  case M680x0::COND_EQ: return M680x0::COND_NE;
  case M680x0::COND_VC: return M680x0::COND_VS;
  case M680x0::COND_VS: return M680x0::COND_VC;
  case M680x0::COND_PL: return M680x0::COND_MI;
  case M680x0::COND_MI: return M680x0::COND_PL;
  case M680x0::COND_GE: return M680x0::COND_LT;
  case M680x0::COND_LT: return M680x0::COND_GE;
  case M680x0::COND_GT: return M680x0::COND_LE;
  case M680x0::COND_LE: return M680x0::COND_GT;
  }
}

static inline unsigned
GetCondBranchFromCond(M680x0::CondCode CC) {
  switch (CC) {
  default: llvm_unreachable("Illegal condition code!");
  case M680x0::COND_EQ: return M680x0::Beq8;
  case M680x0::COND_NE: return M680x0::Bne8;
  case M680x0::COND_LT: return M680x0::Blt8;
  case M680x0::COND_LE: return M680x0::Ble8;
  case M680x0::COND_GT: return M680x0::Bgt8;
  case M680x0::COND_GE: return M680x0::Bge8;
  case M680x0::COND_CS: return M680x0::Bcs8;
  case M680x0::COND_LS: return M680x0::Bls8;
  case M680x0::COND_HI: return M680x0::Bhi8;
  case M680x0::COND_CC: return M680x0::Bcc8;
  case M680x0::COND_MI: return M680x0::Bmi8;
  case M680x0::COND_PL: return M680x0::Bpl8;
  case M680x0::COND_VS: return M680x0::Bvs8;
  case M680x0::COND_VC: return M680x0::Bvc8;
  }
}

// FIXME would be nice tablegen to generate these predicates, mb tag based

static inline unsigned IsCMP(unsigned Op) {
  switch (Op) {
  default: return false;
  case M680x0::CMP8dd:
  case M680x0::CMP8df:
  case M680x0::CMP8di:
  case M680x0::CMP8dj:
  case M680x0::CMP8dp:
  case M680x0::CMP16dd:
  case M680x0::CMP16df:
  case M680x0::CMP16di:
  case M680x0::CMP16dj:
  case M680x0::CMP16dp:
    return true;
  }
}

static inline bool IsSETCC(unsigned SETCC) {
  switch (SETCC) {
  default: return false;
  case M680x0::SETd8eq:
  case M680x0::SETd8ne:
  case M680x0::SETd8lt:
  case M680x0::SETd8ge:
  case M680x0::SETd8le:
  case M680x0::SETd8gt:
  case M680x0::SETd8cs:
  case M680x0::SETd8cc:
  case M680x0::SETd8ls:
  case M680x0::SETd8hi:
  case M680x0::SETd8pl:
  case M680x0::SETd8mi:
  case M680x0::SETd8vc:
  case M680x0::SETd8vs:
  case M680x0::SETj8eq:
  case M680x0::SETj8ne:
  case M680x0::SETj8lt:
  case M680x0::SETj8ge:
  case M680x0::SETj8le:
  case M680x0::SETj8gt:
  case M680x0::SETj8cs:
  case M680x0::SETj8cc:
  case M680x0::SETj8ls:
  case M680x0::SETj8hi:
  case M680x0::SETj8pl:
  case M680x0::SETj8mi:
  case M680x0::SETj8vc:
  case M680x0::SETj8vs:
  case M680x0::SETp8eq:
  case M680x0::SETp8ne:
  case M680x0::SETp8lt:
  case M680x0::SETp8ge:
  case M680x0::SETp8le:
  case M680x0::SETp8gt:
  case M680x0::SETp8cs:
  case M680x0::SETp8cc:
  case M680x0::SETp8ls:
  case M680x0::SETp8hi:
  case M680x0::SETp8pl:
  case M680x0::SETp8mi:
  case M680x0::SETp8vc:
  case M680x0::SETp8vs:
     return true;
  }
}

} /* M680x0  */

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

  unsigned RemoveBranch(MachineBasicBlock &MBB) const override;
  unsigned InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
                        const DebugLoc &DL) const override;

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

  // Add appropriate SExt nodes
  void AddSExt(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, DebugLoc DL,
               unsigned Reg, MVT From, MVT To) const;

  // Add appropriate ZExt nodes
  void AddZExt(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, DebugLoc DL,
               unsigned Reg, MVT From, MVT To) const;

  // Move across register classes without extension
  bool ExpandMOVX_RR(MachineInstrBuilder &MIB, MVT MVTDst, MVT MVTSrc) const;

  // Move from register and extend
  bool ExpandMOVSZX_RR(MachineInstrBuilder &MIB, bool isSigned,
                      MVT MVTDst, MVT MVTSrc) const;

  // Move from memory and extend
  bool ExpandMOVSZX_RM(MachineInstrBuilder &MIB, bool isSigned,
                     const MCInstrDesc &Desc,
                     MVT MVTDst, MVT MVTSrc) const;

  // Push/Pop to/from stack
  bool ExpandPUSH_POP(MachineInstrBuilder &MIB, const MCInstrDesc &Desc,
                      bool isPush) const;

  // Moves to/from CCR
  bool ExpandCCR(MachineInstrBuilder &MIB, bool isToCCR) const;

  // Expand all MOVEM pseudos into real MOVEMs
  bool ExpandMOVEM(MachineInstrBuilder &MIB, const MCInstrDesc &Desc,
                   bool isRM) const;

  /// getGlobalBaseReg - Return a virtual register initialized with the
  /// the global base register value. Output instructions required to
  /// initialize the register in the function entry block, if necessary.
  unsigned getGlobalBaseReg(MachineFunction *MF) const;

  std::pair<unsigned, unsigned>
  decomposeMachineOperandsTargetFlags(unsigned TF) const override;

  ArrayRef<std::pair<unsigned, const char *>>
  getSerializableDirectMachineOperandTargetFlags() const override;
};

} // namespace llvm

#endif
