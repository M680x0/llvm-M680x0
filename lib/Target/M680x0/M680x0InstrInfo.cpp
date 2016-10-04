//===-- M680x0InstrInfo.cpp - M680x0 Instruction Information --------------===//
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

#include "M680x0InstrInfo.h"

#include "M680x0TargetMachine.h"
#include "M680x0MachineFunction.h"
#include "M680x0InstrBuilder.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/LiveVariables.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "M680x0-instr-info"

#define GET_INSTRINFO_CTOR_DTOR
#include "M680x0GenInstrInfo.inc"

// Pin the vtable to this file.
void M680x0InstrInfo::anchor() {}

M680x0InstrInfo::M680x0InstrInfo(const M680x0Subtarget &STI)
    : Subtarget(STI), RI(STI) {}

/// Expand SExt MOVE pseudos into a MOV and a EXT if the operands are two
/// different registers or just EXT if it is the same register
bool M680x0InstrInfo::
ExpandMOVSZX_RR(MachineInstrBuilder &MIB, bool isSigned,
               MVT MVTDst, MVT MVTSrc) const {
  DEBUG(dbgs() << "Expand " << *MIB.getInstr() << " to ");

  unsigned SubIdx;
  unsigned Ext;
  unsigned Move;

  if (MVTDst == MVT::i16) {
    SubIdx = M680x0::MxSubRegIndex8Lo;
    Ext = M680x0::EXT16;
    Move = M680x0::MOV16rr;
  } else { // i32
    SubIdx = M680x0::MxSubRegIndex16Lo;
    Ext = M680x0::EXT32;
    Move = M680x0::MOV32rr;
  }

  unsigned Dst = MIB->getOperand(0).getReg();
  unsigned Src = MIB->getOperand(1).getReg();

  assert (Dst != Src && "You cannot use the same Regs with MOVSX_RR");

  auto TRI = getRegisterInfo();

  auto RCDst = TRI.getMinimalPhysRegClass(Dst, MVTDst);
  auto RCSrc = TRI.getMinimalPhysRegClass(Dst, MVTSrc);

  assert (RCDst && RCSrc && "Wrong use of MOVSX_RR");
  assert (RCDst != RCSrc && "You cannot use the same Reg Classes with MOVSX_RR");

  // We need to find the super source register that matches the size of Dst
  unsigned SSrc = TRI.getMatchingSuperReg( Src, SubIdx, RCSrc);

  MachineBasicBlock &MBB = *MIB->getParent();
  DebugLoc DL = MIB->getDebugLoc();

  if (isSigned) { 
    // If it happens to that super source register is the destination register
    // we just issue an extension on it
    if (Dst == SSrc) {
      DEBUG(dbgs() << "Extension" << '\n');
      BuildMI(MBB, MIB.getInstr(), DL, get(Ext), Dst) .addReg(SSrc);
    } else { // otherwise we need to copy first
      DEBUG(dbgs() << "Copy and Extension" << '\n');
      BuildMI(MBB, MIB.getInstr(), DL, get(Move), Dst) .addReg(SSrc);
      BuildMI(MBB, MIB.getInstr(), DL, get(Ext),  Dst) .addReg(SSrc);
    }
  } else {
    // requires BFCLR
    llvm_unreachable("MOVX_RR is not implemented");
  }

  MIB->eraseFromParent();

  return true;
}

bool M680x0InstrInfo::
ExpandMOVSZX_RM(MachineInstrBuilder &MIB, bool isSigned,
              const MCInstrDesc &Desc,
              MVT MVTDst, MVT MVTSrc) const {
  DEBUG(dbgs() << "Expand " << *MIB.getInstr() << " to MOV and ");

  // Make this a plain move
  MIB->setDesc(Desc);

  MachineBasicBlock::iterator I = MIB.getInstr(); I++;
  MachineBasicBlock &MBB = *MIB->getParent();
  DebugLoc DL = MIB->getDebugLoc();

  unsigned Dst = MIB->getOperand(0).getReg();

  if (isSigned) {
    DEBUG(dbgs() << "Extension" << '\n');
    unsigned Ext = MVTDst == MVT::i16 ? M680x0::EXT16 : M680x0::EXT32;
    BuildMI(MBB, I, DL, get(Ext), Dst).addReg(Dst);
  } else {
    DEBUG(dbgs() << "Bitfield Clear" << '\n');
    // requires BFCLR
    llvm_unreachable("MOVX_RR is not implemented");
  }

  return true;
}

bool M680x0InstrInfo::
ExpandMOVX_RR(MachineInstrBuilder &MIB, const MCInstrDesc &Desc,
              MVT MVTDst, MVT MVTSrc) const {
  unsigned SubIdx;

  if (MVTDst == MVT::i16) {
    SubIdx = M680x0::MxSubRegIndex8Lo;
  } else { // i32
    SubIdx = M680x0::MxSubRegIndex16Lo;
  }

  unsigned Dst = MIB->getOperand(0).getReg();
  unsigned Src = MIB->getOperand(1).getReg();

  assert (Dst != Src && "You cannot use the same Regs with MOVX_RR");

  auto TRI = getRegisterInfo();

  auto RCDst = TRI.getMinimalPhysRegClass(Dst, MVTDst);
  auto RCSrc = TRI.getMinimalPhysRegClass(Dst, MVTSrc);

  assert (RCDst && RCSrc && "Wrong use of MOVX_RR");
  assert (RCDst != RCSrc && "You cannot use the same Reg Classes with MOVX_RR");

  // We need to find the super source register that matches the size of Dst
  unsigned SSrc = TRI.getMatchingSuperReg( Src, SubIdx, RCSrc);

  DebugLoc DL = MIB->getDebugLoc();

  // If it happens to that super source register is the destination register
  // we do nothing
  if (Dst == SSrc) {
    DEBUG(dbgs() << "Remove " << *MIB.getInstr() << '\n');
    MIB->eraseFromParent();
  } else { // otherwise we need to MOV
    DEBUG(dbgs() << "Expand " << *MIB.getInstr() << " to MOV\n");
    MIB->setDesc(Desc);
  }

  return true;
}

bool M680x0InstrInfo::
expandPostRAPseudo(MachineInstr &MI) const {
  MachineInstrBuilder MIB(*MI.getParent()->getParent(), MI);
  switch (MI.getOpcode()) {
    // TODO would be nice to infer all these parameters
    case M680x0::MOVSXd16d8:
      return ExpandMOVSZX_RR(MIB, true, MVT::i16, MVT::i8);
    case M680x0::MOVSXd32d8:
      return ExpandMOVSZX_RR(MIB, true, MVT::i32, MVT::i8);
    case M680x0::MOVSXd32d16:
      return ExpandMOVSZX_RR(MIB, true, MVT::i32, MVT::i16);

    case M680x0::MOVZXd16d8:
      return ExpandMOVSZX_RR(MIB, false, MVT::i16, MVT::i8);
    case M680x0::MOVZXd32d8:
      return ExpandMOVSZX_RR(MIB, false, MVT::i32, MVT::i8);
    case M680x0::MOVZXd32d16:
      return ExpandMOVSZX_RR(MIB, false, MVT::i32, MVT::i16);

    case M680x0::MOVSXd16j8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dj), MVT::i16, MVT::i8);
    case M680x0::MOVSXd32j8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dj), MVT::i32, MVT::i8);
    case M680x0::MOVSXd32j16:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV16dj), MVT::i32, MVT::i16);

    case M680x0::MOVZXd16j8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dj), MVT::i16, MVT::i8);
    case M680x0::MOVZXd32j8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dj), MVT::i32, MVT::i8);
    case M680x0::MOVZXd32j16:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV16dj), MVT::i32, MVT::i16);

    case M680x0::MOVSXd16p8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dp), MVT::i16, MVT::i8);
    case M680x0::MOVSXd32p8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dp), MVT::i32, MVT::i8);
    case M680x0::MOVSXd32p16:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV16dp), MVT::i32, MVT::i16);

    case M680x0::MOVZXd16p8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dp), MVT::i16, MVT::i8);
    case M680x0::MOVZXd32p8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8dp), MVT::i32, MVT::i8);
    case M680x0::MOVZXd32p16:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV16dp), MVT::i32, MVT::i16);


    case M680x0::MOVSXd16f8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8df), MVT::i16, MVT::i8);
    case M680x0::MOVSXd32f8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8df), MVT::i32, MVT::i8);
    case M680x0::MOVSXd32f16:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV16df), MVT::i32, MVT::i16);

    case M680x0::MOVZXd16f8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8df), MVT::i16, MVT::i8);
    case M680x0::MOVZXd32f8:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV8df), MVT::i32, MVT::i8);
    case M680x0::MOVZXd32f16:
      return ExpandMOVSZX_RM(MIB, true, get(M680x0::MOV16df), MVT::i32, MVT::i16);

    case M680x0::MOVXd16d8:
      return ExpandMOVX_RR(MIB, get(M680x0::MOV8df), MVT::i16, MVT::i8);
    case M680x0::MOVXd32d8:
      return ExpandMOVX_RR(MIB, get(M680x0::MOV8df), MVT::i32, MVT::i8);
    case M680x0::MOVXd32d16:
      return ExpandMOVX_RR(MIB, get(M680x0::MOV16df), MVT::i32, MVT::i16);
  }
  return false;
}

void M680x0InstrInfo::
copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
            const DebugLoc &DL, unsigned DestReg,
            unsigned SrcReg, bool KillSrc) const {
  unsigned Opc = 0;

  // First deal with the normal symmetric copies.
  if (M680x0::XR32RegClass.contains(DestReg, SrcReg))
    Opc = M680x0::MOV32rr;
  else if (M680x0::XR16RegClass.contains(DestReg, SrcReg))
    Opc = M680x0::MOV16rr;
  else if (M680x0::DR8RegClass.contains(DestReg, SrcReg)) {
    Opc = M680x0::MOV8dd;
  }

  if (Opc) {
    BuildMI(MBB, MI, DL, get(Opc), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }

  bool FromCCR = SrcReg == M680x0::CCR;
  bool FromSR = SrcReg == M680x0::SR;
  bool ToCCR = DestReg == M680x0::CCR;
  bool ToSR = DestReg == M680x0::SR;

  if (FromCCR || ToCCR) {
    // TODO
  } else if (FromSR || ToSR) {
    // TODO
  }

  DEBUG(dbgs() << "Cannot copy " << RI.getName(SrcReg)
               << " to " << RI.getName(DestReg) << '\n');
  llvm_unreachable("Cannot emit physreg copy instruction");
}

static unsigned getLoadStoreRegOpcode(unsigned Reg,
                                      const TargetRegisterClass *RC,
                                      const M680x0Subtarget &STI,
                                      bool load) {
  switch (RC->getSize()) {
  default:
    llvm_unreachable("Unknown spill size");
  case 1:
    assert(M680x0::DR8RegClass.hasSubClassEq(RC) && "Unknown 1-byte regclass");
    return load ? M680x0::MOV8dp : M680x0::MOV8pd;
  case 2:
    assert(M680x0::XR16RegClass.hasSubClassEq(RC) && "Unknown 2-byte regclass");
    return load ? M680x0::MOV16dp : M680x0::MOV16pd;
  case 4:
    assert(M680x0::XR16RegClass.hasSubClassEq(RC) && "Unknown 4-byte regclass");
    return load ? M680x0::MOV32dp : M680x0::MOV32pd;
  }
}

static unsigned getStoreRegOpcode(unsigned SrcReg,
                                  const TargetRegisterClass *RC,
                                  const M680x0Subtarget &STI) {
  return getLoadStoreRegOpcode(SrcReg, RC, STI, false);
}

static unsigned getLoadRegOpcode(unsigned DestReg,
                                 const TargetRegisterClass *RC,
                                 const M680x0Subtarget &STI) {
  return getLoadStoreRegOpcode(DestReg, RC, STI, true);
}

void M680x0InstrInfo::
storeRegToStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                    unsigned SrcReg, bool isKill, int FI,
                    const TargetRegisterClass *RC,
                    const TargetRegisterInfo *TRI) const {
  const MachineFunction &MF = *MBB.getParent();
  assert(MF.getFrameInfo().getObjectSize(FI) >= RC->getSize() &&
         "Stack slot too small for store");
  unsigned Opc = getStoreRegOpcode(SrcReg, RC, Subtarget);
  DebugLoc DL = MBB.findDebugLoc(MI);
  // (0,FI) <- $reg
  addFrameReference(BuildMI(MBB, MI, DL, get(Opc)), FI)
    .addReg(SrcReg, getKillRegState(isKill));
}

void M680x0InstrInfo::
loadRegFromStackSlot(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                     unsigned DestReg, int FI,
                     const TargetRegisterClass *RC,
                     const TargetRegisterInfo *TRI) const {
  unsigned Opc = getLoadRegOpcode(DestReg, RC, Subtarget);
  DebugLoc DL = MBB.findDebugLoc(MI);
  addFrameReference(BuildMI(MBB, MI, DL, get(Opc), DestReg), FI);
}
