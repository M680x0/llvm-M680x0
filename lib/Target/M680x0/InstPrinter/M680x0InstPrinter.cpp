//===-- M680x0InstPrinter.cpp - Convert M680x0 MCInst to assembly syntax --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an M680x0 MCInst to a .s file.
//
//===----------------------------------------------------------------------===//
//
// TODO finish printer, it does not conform to Motorola asm at all

#include "M680x0InstPrinter.h"

#include "M680x0InstrInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define PRINT_ALIAS_INSTR
#include "M680x0GenAsmWriter.inc"

void M680x0InstPrinter::
printRegName(raw_ostream &OS, unsigned RegNo) const {
  OS << "%" << StringRef(getRegisterName(RegNo));
}

void M680x0InstPrinter::
printInst(const MCInst *MI, raw_ostream &O, StringRef Annot,
                                            const MCSubtargetInfo &STI) {
  if (!printAliasInstr(MI, O)) {
    printInstruction(MI, O);
  }
  printAnnotation(O, Annot);
}

void M680x0InstPrinter::
printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }

  if (Op.isImm()) {
    O << '#' << Op.getImm();
    return;
  }

  assert(Op.isExpr() && "unknown operand kind in printOperand");
  Op.getExpr()->print(O, &MAI);
}

void M680x0InstPrinter::
printUnsignedImm(const MCInst *MI, int opNum, raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(opNum);
  if (MO.isImm())
    O << '#' << (unsigned short int)MO.getImm();
  else
    printOperand(MI, opNum, O);
}

void M680x0InstPrinter::
printDisp(const MCInst *MI, int opNum, raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(opNum);
  if (Op.isImm()) {
    O << Op.getImm();
      return;
  }
  assert(Op.isExpr() && "unknown operand kind in printOperand");
  Op.getExpr()->print(O, &MAI);
}

void M680x0InstPrinter::
printARIMem(const MCInst *MI, int opNum, raw_ostream &O) {
  O << '(';
  printOperand(MI, opNum, O);
  O << ')';
}

void M680x0InstPrinter::
printARIPIMem(const MCInst *MI, int opNum, raw_ostream &O) {
  O << "(";
  printOperand(MI, opNum, O);
  O << ")+";
}

void M680x0InstPrinter::
printARIPDMem(const MCInst *MI, int opNum, raw_ostream &O) {
  O << "-(";
  printOperand(MI, opNum, O);
  O << ")";
}

void M680x0InstPrinter::
printARIDMem(const MCInst *MI, int opNum, raw_ostream &O) {
  O << '(';
  printDisp(MI, opNum + M680x0::MemDisp, O);
  O << ',';
  printOperand(MI, opNum + M680x0::MemBase, O);
  O << ')';
}

void M680x0InstPrinter::
printARIIMem(const MCInst *MI, int opNum, raw_ostream &O) {
    // TODO print (i,An,Rn.W)
    // HMM... is it allowed for M68000 to use this form?
}

// NOTE forcing (W,L) size available since M68020 only
void M680x0InstPrinter::
printAbsMem(const MCInst *MI, int opNum, raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(opNum);
  if (MO.isImm()) {
    // ??? Print it in hex?
    O << (unsigned short int)MO.getImm();
  }
  else {
    printOperand(MI, opNum, O);
  }
}

void M680x0InstPrinter::
printPCDMem(const MCInst *MI, int opNum, raw_ostream &O) {
  O << '(';
  printDisp(MI, opNum + M680x0::PCRelDisp, O);
  O << ",%pc)";
}

void M680x0InstPrinter::
printPCIMem(const MCInst *MI, int opNum, raw_ostream &O) {
    // TODO print (i,PC,Rn.W)
    // HMM... is it allowed for M68000 to use this form?
}
