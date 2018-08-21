//===--- createM680x0CollapseMOVEMPass.cpp - Expand pseudo instructions ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass collapse sequential MOVEM instructions into a single one.
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
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/GlobalValue.h"
using namespace llvm;

#define DEBUG_TYPE "M680x0-collapse-movem"

namespace {

enum UpdateType { Ascending, Descending, Intermixed };

struct MOVEMState {
private:
  MachineBasicBlock::iterator Begin;
  MachineBasicBlock::iterator End;

  unsigned Base;

  int Start;
  int Stop;

  unsigned Mask;

  enum { None, Load, Store } Type;

public:
  MOVEMState()
      : Begin(nullptr), End(nullptr), Base(0), Start(INT_MIN), Stop(INT_MAX),
        Mask(0), Type(None) {}

  void SetBegin(MachineBasicBlock::iterator &MI) {
    assert(Begin == nullptr);
    Begin = MI;
  }

  void SetEnd(MachineBasicBlock::iterator &MI) {
    assert(End == nullptr);
    End = MI;
  }

  bool HasBase() { return Base != 0; }

  unsigned GetBase() {
    assert(Base);
    return Base;
  }

  MachineBasicBlock::iterator GetBegin() {
    assert(Begin != nullptr);
    return Begin;
  }

  MachineBasicBlock::iterator GetEnd() {
    assert(End != nullptr);
    return End;
  }

  unsigned GetMask() { return Mask; }

  void SetBase(int Value) {
    assert(!HasBase());
    Base = Value;
  }

  // You need to call this before Mask update
  UpdateType ClassifyUpdateByMask(unsigned Value) {
    assert(Value);

    if (Mask == 0) {
      return Ascending;
    } else if (Mask < Value) {
      return Ascending;
    } else if (Mask > Value) {
      return Descending;
    }

    return Intermixed;
  }

  bool Update(int O, int M) {
    UpdateType Type = ClassifyUpdateByMask(M);
    // assert(Type != Intermixed);
    if (Type == Intermixed)
      return false;
    if (Start == INT_MIN) {
      Start = Stop = O;
      UpdateMask(M);
      return true;
    } else if (Type == Descending && O == Start - 4) {
      Start -= 4;
      UpdateMask(M);
      return true;
    } else if (Type == Ascending && O == Stop + 4) {
      Stop += 4;
      UpdateMask(M);
      return true;
    }

    return false;
  }

  int GetFinalOffset() {
    // Since MOVEM in control mode increment the address on each iteration
    assert(Start != INT_MIN);
    return Start;
  }

  bool UpdateMask(unsigned Value) {
    assert(Value == (Value & 0xFFFF) && "Mask must fit 16 bit");
    assert(!(Value & Mask) &&
           "This is weird, there should be no intersections");
    Mask |= Value;
    return true;
  }

  void SetLoad() { Type = Load; }
  void SetStore() { Type = Store; }

  bool IsLoad() { return Type == Load; }
  bool IsStore() { return Type == Store; }
};

class M680x0CollapseMOVEM : public MachineFunctionPass {
public:
  static char ID;

  const M680x0Subtarget *STI;
  const M680x0InstrInfo *TII;
  const M680x0RegisterInfo *TRI;
  const M680x0MachineFunctionInfo *MFI;
  const M680x0FrameLowering *FL;

  M680x0CollapseMOVEM() : MachineFunctionPass(ID) {}

  void Finish(MachineBasicBlock &MBB, MOVEMState &State) {
    auto MI = State.GetBegin();
    auto End = State.GetEnd();
    auto DL = MI->getDebugLoc();

    // No need to delete then add a single instruction
    if (std::next(MI) == End) {
      State = MOVEMState();
      return;
    }

    // Delete all the MOVEM instruction till the end
    while (MI != End) {
      auto Next = std::next(MI);
      MBB.erase(MI);
      MI = Next;
    }

    // Add a unified one
    if (State.IsLoad()) {
      BuildMI(MBB, End, DL, TII->get(M680x0::MOVM32mp))
          .addImm(State.GetMask())
          .addImm(State.GetFinalOffset())
          .addReg(State.GetBase());
    } else {
      BuildMI(MBB, End, DL, TII->get(M680x0::MOVM32pm))
          .addImm(State.GetFinalOffset())
          .addReg(State.GetBase())
          .addImm(State.GetMask());
    }

    State = MOVEMState();
  }

  bool ProcessMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                 MOVEMState &State, unsigned Mask, int Offset, unsigned Reg,
                 bool IsStore = false) {
    if (State.HasBase()) {
      // If current Type, Reg, Offset and Mask is in proper order  then
      // merge in the state
      MOVEMState Temp = State;
      if (State.IsStore() == IsStore && State.GetBase() == Reg &&
          State.Update(Offset, Mask)) {
        return true;
        // Otherwise we Finish processing of the current MOVEM sequance and
        // start a new one
      } else {
        State = Temp;
        State.SetEnd(MI);
        Finish(MBB, State);
        return ProcessMI(MBB, MI, State, Mask, Offset, Reg, IsStore);
      }
      // If this is the first instruction is sequance then initialize the State
    } else if (Reg == TRI->getStackRegister() ||
               Reg == TRI->getBaseRegister() ||
               Reg == TRI->getFrameRegister(*MBB.getParent())) {
      State.SetBegin(MI);
      State.SetBase(Reg);
      State.Update(Offset, Mask);
      IsStore ? State.SetStore() : State.SetLoad();
      return true;
    }
    return false;
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    STI = &static_cast<const M680x0Subtarget &>(MF.getSubtarget());
    TII = STI->getInstrInfo();
    TRI = STI->getRegisterInfo();
    MFI = MF.getInfo<M680x0MachineFunctionInfo>();
    FL = STI->getFrameLowering();

    bool Modified = false;

    MOVEMState State;

    unsigned Mask = 0;
    unsigned Reg = 0;
    int Offset = 0;

    for (auto &MBB : MF) {
      auto MI = MBB.begin(), E = MBB.end();
      while (MI != E) {
        // Processing might change current instruction, save next first
        auto NMI = std::next(MI);
        switch (MI->getOpcode()) {
        default:
          if (State.HasBase()) {
            State.SetEnd(MI);
            Finish(MBB, State);
            Modified = true;
          }
          break;
        case M680x0::MOVM32jm:
          Mask = MI->getOperand(1).getImm();
          Reg = MI->getOperand(0).getReg();
          Offset = 0;
          Modified |= ProcessMI(MBB, MI, State, Mask, Offset, Reg, true);
          break;
        case M680x0::MOVM32pm:
          Mask = MI->getOperand(2).getImm();
          Reg = MI->getOperand(1).getReg();
          Offset = MI->getOperand(0).getImm();
          Modified |= ProcessMI(MBB, MI, State, Mask, Offset, Reg, true);
          break;
        case M680x0::MOVM32mj:
          Mask = MI->getOperand(0).getImm();
          Reg = MI->getOperand(1).getReg();
          Offset = 0;
          Modified |= ProcessMI(MBB, MI, State, Mask, Offset, Reg, false);
          break;
        case M680x0::MOVM32mp:
          Mask = MI->getOperand(0).getImm();
          Reg = MI->getOperand(2).getReg();
          Offset = MI->getOperand(1).getImm();
          Modified |= ProcessMI(MBB, MI, State, Mask, Offset, Reg, false);
          break;
        }
        MI = NMI;
      }

      if (State.HasBase()) {
        State.SetEnd(MI);
        Finish(MBB, State);
      }
    }

    return Modified;
  }

  StringRef getPassName() const override {
    return "M680x0 MOVEM collapser pass";
  }
};

char M680x0CollapseMOVEM::ID = 0;
} // End anonymous namespace.

/// Returns an instance of the pseudo instruction expansion pass.
FunctionPass *llvm::createM680x0CollapseMOVEMPass() {
  return new M680x0CollapseMOVEM();
}
