//===-- M680x0MCInstLower.cpp - Convert M680x0 MachineInstr to MCInst ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower M680x0 MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "M680x0MCInstLower.h"

#include "M680x0AsmPrinter.h"
#include "M680x0InstrInfo.h"
#include "MCTargetDesc/M680x0BaseInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"

using namespace llvm;

M680x0MCInstLower::
M680x0MCInstLower(MachineFunction &MF, M680x0AsmPrinter &AP)
    : Ctx(MF.getContext()), MF(MF), TM(MF.getTarget()),
      MAI(*TM.getMCAsmInfo()), AsmPrinter(AP) {}

MCSymbol *M680x0MCInstLower::
GetSymbolFromOperand(const MachineOperand &MO) const {
  assert((MO.isGlobal() || MO.isSymbol() || MO.isMBB())
      && "Isn't a symbol reference");

  const DataLayout &DL = MF.getDataLayout();

  MCSymbol *Sym = nullptr;
  SmallString<128> Name;
  StringRef Suffix;

  if (!Suffix.empty())
    Name += DL.getPrivateGlobalPrefix();

  if (MO.isGlobal()) {
    const GlobalValue *GV = MO.getGlobal();
    AsmPrinter.getNameWithPrefix(Name, GV);
  } else if (MO.isSymbol()) {
    Mangler::getNameWithPrefix(Name, MO.getSymbolName(), DL);
  } else if (MO.isMBB()) {
    assert(Suffix.empty());
    Sym = MO.getMBB()->getSymbol();
  }

  Name += Suffix;
  if (!Sym)
    Sym = Ctx.getOrCreateSymbol(Name);

  return Sym;
}

MCOperand M680x0MCInstLower::
LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const {
  // FIXME: We would like an efficient form for this, so we don't have to do a
  // lot of extra uniquing.
  const MCExpr *Expr = nullptr;
  MCSymbolRefExpr::VariantKind RefKind = MCSymbolRefExpr::VK_None;

  switch (MO.getTargetFlags()) {
  default: llvm_unreachable("Unknown target flag on GV operand");
  case M680x0II::MO_NO_FLAG:
  case M680x0II::MO_ABSOLUTE_ADDRESS:
  case M680x0II::MO_PC_RELATIVE_ADDRESS: break;
  case M680x0II::MO_GOTPCREL:  RefKind = MCSymbolRefExpr::VK_GOTPCREL; break;
  case M680x0II::MO_GOT:       RefKind = MCSymbolRefExpr::VK_GOT;      break;
  case M680x0II::MO_GOTOFF:    RefKind = MCSymbolRefExpr::VK_GOTOFF;   break;
  case M680x0II::MO_PLT:       RefKind = MCSymbolRefExpr::VK_PLT;      break;
  }

  if (!Expr) {
    Expr = MCSymbolRefExpr::create(Sym, RefKind, Ctx);
  }

  if (!MO.isJTI() && !MO.isMBB() && MO.getOffset()) {
    Expr = MCBinaryExpr::createAdd(Expr,
           MCConstantExpr::create(MO.getOffset(), Ctx), Ctx);
  }

  return MCOperand::createExpr(Expr);
}

Optional<MCOperand> M680x0MCInstLower::
LowerOperand(const MachineInstr *MI,
             const MachineOperand &MO) const {
  switch (MO.getType()) {
  default:
    MI->dump();
    llvm_unreachable("unknown operand type");
  case MachineOperand::MO_Register:
    // Ignore all implicit register operands.
    if (MO.isImplicit())
      return None;
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_MachineBasicBlock:
  case MachineOperand::MO_GlobalAddress:
  case MachineOperand::MO_ExternalSymbol:
    return LowerSymbolOperand(MO, GetSymbolFromOperand(MO));
  case MachineOperand::MO_MCSymbol:
    return LowerSymbolOperand(MO, MO.getMCSymbol());
  case MachineOperand::MO_JumpTableIndex:
    return LowerSymbolOperand(MO, AsmPrinter.GetJTISymbol(MO.getIndex()));
  case MachineOperand::MO_ConstantPoolIndex:
    return LowerSymbolOperand(MO, AsmPrinter.GetCPISymbol(MO.getIndex()));
  case MachineOperand::MO_BlockAddress:
    return LowerSymbolOperand(
        MO, AsmPrinter.GetBlockAddressSymbol(MO.getBlockAddress()));
  case MachineOperand::MO_RegisterMask:
    // Ignore call clobbers.
    return None;
  }
}

void M680x0MCInstLower::
Lower(const MachineInstr *MI, MCInst &OutMI) const {
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    Optional<MCOperand> MCOp = LowerOperand(MI, MO);

    if (MCOp.hasValue() && MCOp.getValue().isValid())
      OutMI.addOperand(MCOp.getValue());
  }

  switch (OutMI.getOpcode()) {

  // TAILJMPj, TAILJMPq - Lower to the correct jump instructions.
  case M680x0::TAILJMPj:
  case M680x0::TAILJMPq:{
    unsigned Opcode;
    switch (OutMI.getOpcode()) {
    default: llvm_unreachable("Invalid opcode");
    case M680x0::TAILJMPj: Opcode = M680x0::JMP32j; break;
    case M680x0::TAILJMPq: Opcode = M680x0::BRA8; break;
    }

    MCOperand Saved = OutMI.getOperand(0);
    OutMI = MCInst();
    OutMI.setOpcode(Opcode);
    OutMI.addOperand(Saved);
    break;
  }
  }
}
