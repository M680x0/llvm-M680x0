//===------- M680x0ExpandPseudo.cpp - Expand pseudo instructions -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions to allow proper scheduling, if-conversion, other late
// optimizations, or simply the encoding of the instructions.
//
//===----------------------------------------------------------------------===//

#include "M680x0.h"
#include "M680x0FrameLowering.h"
#include "M680x0InstrInfo.h"
#include "M680x0MachineFunction.h"
#include "M680x0Subtarget.h"
#include "llvm/Analysis/EHPersonalities.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/Passes.h" // For IDs of passes that are preserved.
#include "llvm/IR/GlobalValue.h"
using namespace llvm;

#define DEBUG_TYPE "M680x0-pseudo"

namespace {
class M680x0ExpandPseudo : public MachineFunctionPass {
public:
  static char ID;
  M680x0ExpandPseudo() : MachineFunctionPass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addPreservedID(MachineLoopInfoID);
    AU.addPreservedID(MachineDominatorsID);
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  const M680x0Subtarget *STI;
  const M680x0InstrInfo *TII;
  const M680x0RegisterInfo *TRI;
  const M680x0MachineFunctionInfo *MFI;
  const M680x0FrameLowering *FL;

  bool runOnMachineFunction(MachineFunction &Fn) override;

  MachineFunctionProperties getRequiredProperties() const override {
    return MachineFunctionProperties().set(
        MachineFunctionProperties::Property::NoVRegs);
  }

  const char *getPassName() const override {
    return "M680x0 pseudo instruction expansion pass";
  }

private:
  bool ExpandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  bool ExpandMBB(MachineBasicBlock &MBB);
};
char M680x0ExpandPseudo::ID = 0;
} // End anonymous namespace.


/// If \p MBBI is a pseudo instruction, this method expands
/// it to the corresponding (sequence of) actual instruction(s).
/// \returns true if \p MBBI has been expanded.
bool M680x0ExpandPseudo::ExpandMI(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;
  MachineInstrBuilder MIB(*MI.getParent()->getParent(), MI);
  unsigned Opcode = MI.getOpcode();
  DebugLoc DL = MBBI->getDebugLoc();
  switch (Opcode) {
  default:
    return false;
  /// TODO would be nice to infer all these parameters

  case M680x0::MOVXd16d8:
    return TII->ExpandMOVX_RR(MIB, MVT::i16, MVT::i8);
  case M680x0::MOVXd32d8:
    return TII->ExpandMOVX_RR(MIB, MVT::i32, MVT::i8);
  case M680x0::MOVXd32d16:
    return TII->ExpandMOVX_RR(MIB, MVT::i32, MVT::i16);


  case M680x0::MOVSXd16d8:
    return TII->ExpandMOVSZX_RR(MIB, true, MVT::i16, MVT::i8);
  case M680x0::MOVSXd32d8:
    return TII->ExpandMOVSZX_RR(MIB, true, MVT::i32, MVT::i8);
  case M680x0::MOVSXd32d16:
    return TII->ExpandMOVSZX_RR(MIB, true, MVT::i32, MVT::i16);

  case M680x0::MOVZXd16d8:
    return TII->ExpandMOVSZX_RR(MIB, false, MVT::i16, MVT::i8);
  case M680x0::MOVZXd32d8:
    return TII->ExpandMOVSZX_RR(MIB, false, MVT::i32, MVT::i8);
  case M680x0::MOVZXd32d16:
    return TII->ExpandMOVSZX_RR(MIB, false, MVT::i32, MVT::i16);


  case M680x0::MOVSXd16j8:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV8dj), MVT::i16, MVT::i8);
  case M680x0::MOVSXd32j8:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV8dj), MVT::i32, MVT::i8);
  case M680x0::MOVSXd32j16:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV16rj), MVT::i32, MVT::i16);

  case M680x0::MOVZXd16j8:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV8dj), MVT::i16, MVT::i8);
  case M680x0::MOVZXd32j8:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV8dj), MVT::i32, MVT::i8);
  case M680x0::MOVZXd32j16:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV16rj), MVT::i32, MVT::i16);


  case M680x0::MOVSXd16p8:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV8dp), MVT::i16, MVT::i8);
  case M680x0::MOVSXd32p8:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV8dp), MVT::i32, MVT::i8);
  case M680x0::MOVSXd32p16:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV16rp), MVT::i32, MVT::i16);

  case M680x0::MOVZXd16p8:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV8dp), MVT::i16, MVT::i8);
  case M680x0::MOVZXd32p8:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV8dp), MVT::i32, MVT::i8);
  case M680x0::MOVZXd32p16:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV16rp), MVT::i32, MVT::i16);


  case M680x0::MOVSXd16f8:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV8df), MVT::i16, MVT::i8);
  case M680x0::MOVSXd32f8:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV8df), MVT::i32, MVT::i8);
  case M680x0::MOVSXd32f16:
    return TII->ExpandMOVSZX_RM(MIB, true, TII->get(M680x0::MOV16rf), MVT::i32, MVT::i16);

  case M680x0::MOVZXd16f8:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV8df), MVT::i16, MVT::i8);
  case M680x0::MOVZXd32f8:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV8df), MVT::i32, MVT::i8);
  case M680x0::MOVZXd32f16:
    return TII->ExpandMOVSZX_RM(MIB, false, TII->get(M680x0::MOV16rf), MVT::i32, MVT::i16);


  case M680x0::MOV8cd:
    return TII->ExpandCCR(MIB, /* isToCCR */ true);
  case M680x0::MOV8dc:
    return TII->ExpandCCR(MIB, /* isToCCR */ false);


  case M680x0::TCRETURNq:
  case M680x0::TCRETURNj: {
    MachineOperand &JumpTarget = MI.getOperand(0);
    MachineOperand &StackAdjust = MI.getOperand(1);
    assert(StackAdjust.isImm() && "Expecting immediate value.");

    // Adjust stack pointer.
    int StackAdj = StackAdjust.getImm();
    int MaxTCDelta = MFI->getTCReturnAddrDelta();
    int Offset = 0;
    assert(MaxTCDelta <= 0 && "MaxTCDelta should never be positive");

    // Incoporate the retaddr area.
    Offset = StackAdj-MaxTCDelta;
    assert(Offset >= 0 && "Offset should never be negative");

    if (Offset) {
      // Check for possible merge with preceding ADD instruction.
      Offset += FL->mergeSPUpdates(MBB, MBBI, true);
      FL->emitSPUpdate(MBB, MBBI, Offset, /*InEpilogue=*/true);
    }

    // Jump to label or value in register.
    if (Opcode == M680x0::TCRETURNq) {
      MachineInstrBuilder MIB = BuildMI(MBB, MBBI, DL, TII->get(M680x0::TAILJMPq));
      if (JumpTarget.isGlobal()) {
        MIB.addGlobalAddress(JumpTarget.getGlobal(), JumpTarget.getOffset(),
                             JumpTarget.getTargetFlags());
      } else {
        assert(JumpTarget.isSymbol());
        MIB.addExternalSymbol(JumpTarget.getSymbolName(),
                              JumpTarget.getTargetFlags());
      }
    } else {
      BuildMI(MBB, MBBI, DL, TII->get(M680x0::TAILJMPj))
          .addReg(JumpTarget.getReg(), RegState::Kill);
    }

    MachineInstr &NewMI = *std::prev(MBBI);
    NewMI.copyImplicitOps(*MBBI->getParent()->getParent(), *MBBI);

    // Delete the pseudo instruction TCRETURN.
    MBB.erase(MBBI);

    return true;
  }
  case M680x0::RET: {
    // Adjust stack to erase error code
    int64_t StackAdj = MBBI->getOperand(0).getImm();
    MachineInstrBuilder MIB;

    if (StackAdj == 0) {
      MIB = BuildMI(MBB, MBBI, DL, TII->get(M680x0::RTS));
    } else if (isUInt<16>(StackAdj)) {
      assert(false &&
             "not implemented, RTD is available since M68020 i think");
      // MIB = BuildMI(MBB, MBBI, DL, TII->get(M680x0::RTD)).addImm(StackAdj);
    } else {
      assert(false &&
             "not implemented, RTD is available since M68020 i think");
      // RTD can only handle immediates as big as 2**16-1.  If we need to pop
      // off bytes before the return address, we must do it manually.
      //
      // BuildMI(MBB, MBBI, DL, TII->get(M680x0::POP32r)).addReg(M680x0::ECX, RegState::Define);
      // FL->emitSPUpdate(MBB, MBBI, StackAdj, #<{(|InEpilogue=|)}>#true);
      // BuildMI(MBB, MBBI, DL, TII->get(M680x0::PUSH32r)).addReg(M680x0::ECX);
      // MIB = BuildMI(MBB, MBBI, DL, TII->get(M680x0::RETL));
    }

    // ??? The rest can be ignored?
    // for (unsigned I = 1, E = MBBI->getNumOperands(); I != E; ++I)
    //   MIB.addOperand(MBBI->getOperand(I));
    MBB.erase(MBBI);
    return true;
  }
  }
  llvm_unreachable("Previous switch has a fallthrough?");
}

/// Expand all pseudo instructions contained in \p MBB.
/// \returns true if any expansion occurred for \p MBB.
bool M680x0ExpandPseudo::ExpandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  // MBBI may be invalidated by the expansion.
  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= ExpandMI(MBB, MBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool M680x0ExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  STI = &static_cast<const M680x0Subtarget &>(MF.getSubtarget());
  TII = STI->getInstrInfo();
  TRI = STI->getRegisterInfo();
  MFI = MF.getInfo<M680x0MachineFunctionInfo>();
  FL = STI->getFrameLowering();

  bool Modified = false;
  for (MachineBasicBlock &MBB : MF)
    Modified |= ExpandMBB(MBB);
  return Modified;
}

/// Returns an instance of the pseudo instruction expansion pass.
FunctionPass *llvm::createM680x0ExpandPseudoPass() {
  return new M680x0ExpandPseudo();
}
