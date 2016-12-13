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
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "M680x0-instr-info"

#define GET_INSTRINFO_CTOR_DTOR
#include "M680x0GenInstrInfo.inc"

// Pin the vtable to this file.
void M680x0InstrInfo::anchor() {}

M680x0InstrInfo::
M680x0InstrInfo(const M680x0Subtarget &STI)
    : M680x0GenInstrInfo(M680x0::ADJCALLSTACKDOWN, M680x0::ADJCALLSTACKUP,
                         0, M680x0::RET),
    Subtarget(STI), RI(STI) {}

static M680x0::CondCode getCondFromBranchOpc(unsigned BrOpc) {
  switch (BrOpc) {
  default: return M680x0::COND_INVALID;
  case M680x0::Beq8: return M680x0::COND_EQ;
  case M680x0::Bne8: return M680x0::COND_NE;
  case M680x0::Blt8: return M680x0::COND_LT;
  case M680x0::Ble8: return M680x0::COND_LE;
  case M680x0::Bgt8: return M680x0::COND_GT;
  case M680x0::Bge8: return M680x0::COND_GE;
  case M680x0::Bcs8: return M680x0::COND_CS;
  case M680x0::Bls8: return M680x0::COND_LS;
  case M680x0::Bhi8: return M680x0::COND_HI;
  case M680x0::Bcc8: return M680x0::COND_CC;
  case M680x0::Bmi8: return M680x0::COND_MI;
  case M680x0::Bpl8: return M680x0::COND_PL;
  case M680x0::Bvs8: return M680x0::COND_VS;
  case M680x0::Bvc8: return M680x0::COND_VC;
  }
}

unsigned M680x0InstrInfo::
RemoveBranch(MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;

  while (I != MBB.begin()) {
    --I;
    if (I->isDebugValue())
      continue;
    if (I->getOpcode() != M680x0::BRA8 &&
        getCondFromBranchOpc(I->getOpcode()) == M680x0::COND_INVALID)
      break;
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }

  return Count;
}

unsigned M680x0InstrInfo::
InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
             MachineBasicBlock *FBB, ArrayRef<MachineOperand> Cond,
             const DebugLoc &DL) const {
  // Shouldn't be a fall through.
  assert(TBB && "InsertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 1 || Cond.size() == 0) &&
         "M680x0 branch conditions have one component!");

  if (Cond.empty()) {
    // Unconditional branch?
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(M680x0::BRA8)).addMBB(TBB);
    return 1;
  }

  // If FBB is null, it is implied to be a fall-through block.
  bool FallThru = FBB == nullptr;

  // Conditional branch.
  unsigned Count = 0;
  M680x0::CondCode CC = (M680x0::CondCode)Cond[0].getImm();
  unsigned Opc = GetCondBranchFromCond(CC);
  BuildMI(&MBB, DL, get(Opc)).addMBB(TBB);
  ++Count;
  if (!FallThru) {
    // Two-way Conditional branch. Insert the second branch.
    BuildMI(&MBB, DL, get(M680x0::BRA8)).addMBB(FBB);
    ++Count;
  }
  return Count;
}

void M680x0InstrInfo::
AddSExt(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, DebugLoc DL,
        unsigned Reg, MVT From, MVT To) const {
  if (From == MVT::i8) {
    BuildMI(MBB, I, DL, get(M680x0::EXT16), Reg).addReg(Reg);
  }

  if (To == MVT::i32) {
    BuildMI(MBB, I, DL, get(M680x0::EXT32), Reg).addReg(Reg);
  }
}

void M680x0InstrInfo::
AddZExt(MachineBasicBlock &MBB, MachineBasicBlock::iterator I, DebugLoc DL,
        unsigned Reg, MVT From, MVT To) const {

  unsigned Mask, And;
  if (From == MVT::i8) {
    Mask = 0xFF;
  } else {
    Mask = 0xFFFF;
  }

  if (To == MVT::i16) {
    And = M680x0::AND16di;
  } else { // i32
    And = M680x0::AND32di;
  }

  // TODO use xor r,r to decrease size
  BuildMI(MBB, I, DL, get(And), Reg).addReg(Reg).addImm(Mask);
}

bool M680x0InstrInfo::
ExpandMOVX_RR(MachineInstrBuilder &MIB, MVT MVTDst, MVT MVTSrc) const {
  unsigned SubIdx;

  unsigned Move = MVTDst == MVT::i16 ? M680x0::MOV16rr : M680x0::MOV32rr;

  if (MVTSrc == MVT::i8) {
    SubIdx = M680x0::MxSubRegIndex8Lo;
  } else { // i16
    SubIdx = M680x0::MxSubRegIndex16Lo;
  }

  unsigned Dst = MIB->getOperand(0).getReg();
  unsigned Src = MIB->getOperand(1).getReg();

  assert (Dst != Src && "You cannot use the same Regs with MOVX_RR");

  auto TRI = getRegisterInfo();

  auto RCDst = TRI.getMaximalPhysRegClass(Dst, MVTDst);
  auto RCSrc = TRI.getMaximalPhysRegClass(Src, MVTSrc);

  assert (RCDst && RCSrc && "Wrong use of MOVX_RR");
  assert (RCDst != RCSrc && "You cannot use the same Reg Classes with MOVX_RR");

  // We need to find the super source register that matches the size of Dst
  unsigned SSrc = RI.getMatchingMegaReg( Src, RCDst);
  assert (SSrc && "No viable MEGA register available");

  DebugLoc DL = MIB->getDebugLoc();

  // If it happens to that super source register is the destination register
  // we do nothing
  if (Dst == SSrc) {
    DEBUG(dbgs() << "Remove " << *MIB.getInstr() << '\n');
    MIB->eraseFromParent();
  } else { // otherwise we need to MOV
    DEBUG(dbgs() << "Expand " << *MIB.getInstr() << " to MOV\n");
    MIB->setDesc(get(Move));
    MIB->getOperand(1).setReg(SSrc);
  }

  return true;
}

/// Expand SExt MOVE pseudos into a MOV and a EXT if the operands are two
/// different registers or just EXT if it is the same register
bool M680x0InstrInfo::
ExpandMOVSZX_RR(MachineInstrBuilder &MIB, bool isSigned,
               MVT MVTDst, MVT MVTSrc) const {
  DEBUG(dbgs() << "Expand " << *MIB.getInstr() << " to ");

  unsigned Move;

  if (MVTDst == MVT::i16) {
    Move = M680x0::MOV16rr;
  } else { // i32
    Move = M680x0::MOV32rr;
  }

  unsigned Dst = MIB->getOperand(0).getReg();
  unsigned Src = MIB->getOperand(1).getReg();

  assert (Dst != Src && "You cannot use the same Regs with MOVSX_RR");

  auto TRI = getRegisterInfo();

  auto RCDst = TRI.getMaximalPhysRegClass(Dst, MVTDst);
  auto RCSrc = TRI.getMaximalPhysRegClass(Src, MVTSrc);

  assert (RCDst && RCSrc && "Wrong use of MOVSX_RR");
  assert (RCDst != RCSrc && "You cannot use the same Reg Classes with MOVSX_RR");

  // We need to find the super source register that matches the size of Dst
  unsigned SSrc = RI.getMatchingMegaReg( Src, RCDst);
  assert (SSrc && "No viable MEGA register available");

  MachineBasicBlock &MBB = *MIB->getParent();
  DebugLoc DL = MIB->getDebugLoc();

  if (Dst != SSrc) {
    DEBUG(dbgs() << "Move and " << '\n');
    BuildMI(MBB, MIB.getInstr(), DL, get(Move), Dst).addReg(SSrc);
  }

  if (isSigned) {
    DEBUG(dbgs() << "Sign Extend" << '\n');
    AddSExt(MBB, MIB.getInstr(), DL, Dst, MVTSrc, MVTDst);
  } else {
    DEBUG(dbgs() << "Zero Extend" << '\n');
    AddZExt(MBB, MIB.getInstr(), DL, Dst, MVTSrc, MVTDst);
  }

  MIB->eraseFromParent();

  return true;
}

bool M680x0InstrInfo::
ExpandMOVSZX_RM(MachineInstrBuilder &MIB, bool isSigned,
              const MCInstrDesc &Desc,
              MVT MVTDst, MVT MVTSrc) const {
  DEBUG(dbgs() << "Expand " << *MIB.getInstr() << " to MOV and ");

  unsigned Dst = MIB->getOperand(0).getReg();

  // We need the subreg of Dst to make instruction verifier happy because the
  // real machine instruction consumes and produces values of the same size and
  // the registers the will be used here fall into different classes and this
  // makes IV cry. We could of course use bigger operation but this will put some
  // pressure on cache and memory so no.
  unsigned SubDst = RI.getSubReg(Dst,
      MVTSrc == MVT::i8 ? M680x0::MxSubRegIndex8Lo : M680x0::MxSubRegIndex16Lo);

  // Make this a plain move
  MIB->setDesc(Desc);
  MIB->getOperand(0).setReg(SubDst);

  MachineBasicBlock::iterator I = MIB.getInstr(); I++;
  MachineBasicBlock &MBB = *MIB->getParent();
  DebugLoc DL = MIB->getDebugLoc();

  if (isSigned) {
    DEBUG(dbgs() << "Sign Extend" << '\n');
    AddSExt(MBB, I, DL, Dst, MVTSrc, MVTDst);
  } else {
    DEBUG(dbgs() << "Zero Extend" << '\n');
    AddZExt(MBB, I, DL, Dst, MVTSrc, MVTDst);
  }

  return true;
}

bool M680x0InstrInfo::
ExpandPUSH_POP(MachineInstrBuilder &MIB, const MCInstrDesc &Desc,
               bool isPush) const {
  MachineBasicBlock::iterator I = MIB.getInstr(); I++;
  MachineBasicBlock &MBB = *MIB->getParent();
  MachineOperand MO = MIB->getOperand(0);
  DebugLoc DL = MIB->getDebugLoc();
  if (isPush) {
    BuildMI(MBB, I, DL, Desc).addReg(RI.getStackRegister()).addOperand(MO);
  } else {
    BuildMI(MBB, I, DL, Desc, MO.getReg()).addReg(RI.getStackRegister());
  }
  MIB->eraseFromParent();
  return true;
}

bool M680x0InstrInfo::
ExpandCCR(MachineInstrBuilder &MIB, bool isToCCR) const {
  if (isToCCR) {
    MIB->setDesc(get(M680x0::MOV16cd));
  } else {
    // FIXME M68010 or better is required
    MIB->setDesc(get(M680x0::MOV16dc));
  }
  return true;
}

/// Expand a single-def pseudo instruction to a two-addr
/// instruction with two undef reads of the register being defined.
/// This is used for mapping:
///   %d0 = SETCS_C32d
/// to:
///   %d0 = SUBX32dd %d0<undef>, %d0<undef>
///
static bool Expand2AddrUndef(MachineInstrBuilder &MIB,
                             const MCInstrDesc &Desc) {
  assert(Desc.getNumOperands() == 3 && "Expected two-addr instruction.");
  unsigned Reg = MIB->getOperand(0).getReg();
  MIB->setDesc(Desc);

  // MachineInstr::addOperand() will insert explicit operands before any
  // implicit operands.
  MIB.addReg(Reg, RegState::Undef).addReg(Reg, RegState::Undef);
  // But we don't trust that.
  assert(MIB->getOperand(1).getReg() == Reg &&
         MIB->getOperand(2).getReg() == Reg && "Misplaced operand");
  return true;
}

bool M680x0InstrInfo::
expandPostRAPseudo(MachineInstr &MI) const {
  MachineInstrBuilder MIB(*MI.getParent()->getParent(), MI);
  switch (MI.getOpcode()) {
    case M680x0::PUSH8d:
      return ExpandPUSH_POP(MIB, get(M680x0::MOV8ed), true);
    case M680x0::PUSH16d:
      return ExpandPUSH_POP(MIB, get(M680x0::MOV16er), true);
    case M680x0::PUSH32r:
      return ExpandPUSH_POP(MIB, get(M680x0::MOV32er), true);

    case M680x0::POP8d:
      return ExpandPUSH_POP(MIB, get(M680x0::MOV8do), false);
    case M680x0::POP16d:
      return ExpandPUSH_POP(MIB, get(M680x0::MOV16ro), false);
    case M680x0::POP32r:
      return ExpandPUSH_POP(MIB, get(M680x0::MOV32ro), false);

    case M680x0::SETCS_C8d:
      return Expand2AddrUndef(MIB, get(M680x0::SUBX8dd));
    case M680x0::SETCS_C16d:
      return Expand2AddrUndef(MIB, get(M680x0::SUBX16dd));
    case M680x0::SETCS_C32d:
      return Expand2AddrUndef(MIB, get(M680x0::SUBX32dd));
  }
  return false;
}

void M680x0InstrInfo::
copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
            const DebugLoc &DL, unsigned DstReg,
            unsigned SrcReg, bool KillSrc) const {
  unsigned Opc = 0;

  // First deal with the normal symmetric copies.
  if (M680x0::XR32RegClass.contains(DstReg, SrcReg))
    Opc = M680x0::MOV32rr;
  else if (M680x0::XR16RegClass.contains(DstReg, SrcReg))
    Opc = M680x0::MOV16rr;
  else if (M680x0::DR8RegClass.contains(DstReg, SrcReg)) {
    Opc = M680x0::MOV8dd;
  }

  if (Opc) {
    BuildMI(MBB, MI, DL, get(Opc), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }

  // Now deal with asymmetrically sized copies. The cases that follow are upcast
  // moves.
  //
  // NOTE
  // These moves are not aware of type nature of these values and thus
  // won't do any SExt or ZExt and upper bits will basically contain garbage.
  MachineInstrBuilder MIB(*MBB.getParent(), MI);
  if (M680x0::DR8RegClass.contains(SrcReg)) {
    if (M680x0::XR16RegClass.contains(DstReg)) {
      Opc = M680x0::MOVXd16d8;
    } else if (M680x0::XR32RegClass.contains(DstReg)) {
      Opc = M680x0::MOVXd32d8;
    }
  } else if (M680x0::XR16RegClass.contains(SrcReg)) {
    if (M680x0::XR32RegClass.contains(DstReg)) {
      Opc = M680x0::MOVXd32d16;
    }
  }

  if (Opc) {
    BuildMI(MBB, MI, DL, get(Opc), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }

  bool FromCCR = SrcReg == M680x0::CCR;
  bool FromSR = SrcReg == M680x0::SR;
  bool ToCCR = DstReg == M680x0::CCR;
  bool ToSR = DstReg == M680x0::SR;

  if (FromCCR) {
    assert(M680x0::DR8RegClass.contains(DstReg) && "Need DR8 register to copy CCR");
    Opc = M680x0::MOV8dc;
  } else if (ToCCR) {
    assert(M680x0::DR8RegClass.contains(SrcReg) && "Need DR8 register to copy CCR");
    Opc = M680x0::MOV8cd;
  } else if (FromSR || ToSR) {
    llvm_unreachable("Cannot emit SR copy instruction");
  }

  if (Opc) {
    BuildMI(MBB, MI, DL, get(Opc), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
    return;
  }

  DEBUG(dbgs() << "Cannot copy " << RI.getName(SrcReg)
               << " to " << RI.getName(DstReg) << '\n');
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
    if (M680x0::DR8RegClass.hasSubClassEq(RC)) {
      return load ? M680x0::MOV8dp : M680x0::MOV8pd;
    } else if (M680x0::CCRCRegClass.hasSubClassEq(RC)) {
      return load ? M680x0::MOV16cp : M680x0::MOV16pc;
    }
  llvm_unreachable("Unknown 1-byte regclass");
  case 2:
    assert(M680x0::XR16RegClass.hasSubClassEq(RC) && "Unknown 2-byte regclass");
    return load ? M680x0::MOV16rp : M680x0::MOV16pr;
  case 4:
    assert(M680x0::XR32RegClass.hasSubClassEq(RC) && "Unknown 4-byte regclass");
    return load ? M680x0::MOV32rp : M680x0::MOV32pr;
  }
}

static unsigned getStoreRegOpcode(unsigned SrcReg,
                                  const TargetRegisterClass *RC,
                                  const M680x0Subtarget &STI) {
  return getLoadStoreRegOpcode(SrcReg, RC, STI, false);
}

static unsigned getLoadRegOpcode(unsigned DstReg,
                                 const TargetRegisterClass *RC,
                                 const M680x0Subtarget &STI) {
  return getLoadStoreRegOpcode(DstReg, RC, STI, true);
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
                     unsigned DstReg, int FI,
                     const TargetRegisterClass *RC,
                     const TargetRegisterInfo *TRI) const {
  unsigned Opc = getLoadRegOpcode(DstReg, RC, Subtarget);
  DebugLoc DL = MBB.findDebugLoc(MI);
  addFrameReference(BuildMI(MBB, MI, DL, get(Opc), DstReg), FI);
}

/// Return a virtual register initialized with the
/// the global base register value. Output instructions required to
/// initialize the register in the function entry block, if necessary.
/// TODO: Eliminate this and move the code to M680x0MachineFunctionInfo.
unsigned M680x0InstrInfo::
getGlobalBaseReg(MachineFunction *MF) const {
  M680x0MachineFunctionInfo *MxFI = MF->getInfo<M680x0MachineFunctionInfo>();
  unsigned GlobalBaseReg = MxFI->getGlobalBaseReg();
  if (GlobalBaseReg != 0)
    return GlobalBaseReg;

  // Create the register. The code to initialize it is inserted later,
  // by the CGBR pass (below).
  //
  // NOTE
  // Normally M680x0 uses A5 register as global base pointer but this will
  // create unnecessary spill if we use less then 4 registers in code; since A5
  // is callee-save anyway we could try to allocate caller-save first and if
  // lucky get one, otherwise it does not really matter which callee-save to use.
  MachineRegisterInfo &RegInfo = MF->getRegInfo();
  GlobalBaseReg = RegInfo.createVirtualRegister(&M680x0::AR32_NOSPRegClass);
  MxFI->setGlobalBaseReg(GlobalBaseReg);
  return GlobalBaseReg;
}

std::pair<unsigned, unsigned> M680x0InstrInfo::
decomposeMachineOperandsTargetFlags(unsigned TF) const {
  return std::make_pair(TF, 0u);
}

ArrayRef<std::pair<unsigned, const char *>> M680x0InstrInfo::
getSerializableDirectMachineOperandTargetFlags() const {
  using namespace M680x0II;
  static const std::pair<unsigned, const char *> TargetFlags[] = {
      {MO_ABSOLUTE_ADDRESS,    "M680x0-absolute"},
      {MO_PC_RELATIVE_ADDRESS, "M680x0-pcrel"},
      {MO_GOT,                 "M680x0-got"},
      {MO_GOTOFF,              "M680x0-gotoff"},
      {MO_GOTPCREL,            "M680x0-gotpcrel"},
      {MO_PLT,                 "M680x0-plt"}};
  return makeArrayRef(TargetFlags);
}

namespace {
  /// Create Global Base Reg pass. This initializes the PIC global base register
  struct CGBR : public MachineFunctionPass {
    static char ID;
    CGBR() : MachineFunctionPass(ID) {}

    bool runOnMachineFunction(MachineFunction &MF) override {
      const M680x0Subtarget &STI = MF.getSubtarget<M680x0Subtarget>();
      M680x0MachineFunctionInfo *MxFI = MF.getInfo<M680x0MachineFunctionInfo>();

      unsigned GlobalBaseReg = MxFI->getGlobalBaseReg();

      // If we didn't need a GlobalBaseReg, don't insert code.
      if (GlobalBaseReg == 0)
        return false;

      // Insert the set of GlobalBaseReg into the first MBB of the function
      MachineBasicBlock &FirstMBB = MF.front();
      MachineBasicBlock::iterator MBBI = FirstMBB.begin();
      DebugLoc DL = FirstMBB.findDebugLoc(MBBI);
      const M680x0InstrInfo *TII = STI.getInstrInfo();

      // Generate lea (__GLOBAL_OFFSET_TABLE_,%PC), %A5
      BuildMI(FirstMBB, MBBI, DL, TII->get(M680x0::LEA32q), GlobalBaseReg)
        .addExternalSymbol("_GLOBAL_OFFSET_TABLE_", M680x0II::MO_GOTPCREL);

      return true;
    }

    const char *getPassName() const override {
      return "M680x0 PIC Global Base Reg Initialization";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesCFG();
      MachineFunctionPass::getAnalysisUsage(AU);
    }
  };
}

char CGBR::ID = 0;
FunctionPass* llvm::
createM680x0GlobalBaseRegPass() { return new CGBR(); }
