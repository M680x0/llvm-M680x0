//===-- M680x0AsmBackend.cpp - M680x0 Assembler Backend -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/M680x0BaseInfo.h"
#include "MCTargetDesc/M680x0FixupKinds.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/BinaryFormat/MachO.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCMachObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSectionCOFF.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSectionMachO.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

class M680x0AsmBackend : public MCAsmBackend {

public:
  M680x0AsmBackend(const Target &T) : MCAsmBackend(support::big) {}

  unsigned getNumFixupKinds() const override {
    return llvm::M680x0::NumTargetFixupKinds;
  }

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override {
    unsigned Size = 1 << getFixupKindLog2Size(Fixup.getKind());

    assert(Fixup.getOffset() + Size <= Data.size() && "Invalid fixup offset!");

    // Check that uppper bits are either all zeros or all ones.
    // Specifically ignore overflow/underflow as long as the leakage is
    // limited to the lower bits. This is to remain compatible with
    // other assemblers.
    assert(isIntN(Size * 8 + 1, Value) &&
           "Value does not fit in the Fixup field");

    // Write in Big Endian
    for (unsigned i = 0; i != Size; ++i)
      Data[Fixup.getOffset() + i] = uint8_t(Value >> ((Size - i - 1) * 8));
  }

  bool mayNeedRelaxation(const MCInst &Inst,
                         const MCSubtargetInfo &STI) const override;

  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override;

  void relaxInstruction(const MCInst &Inst, const MCSubtargetInfo &STI,
                        MCInst &Res) const override;

  /// Returns the minimum size of a nop in bytes on this target. The assembler
  /// will use this to emit excess padding in situations where the padding
  /// required for simple alignment would be less than the minimum nop size.
  ///
  unsigned getMinimumNopSize() const override { return 2; }

  /// \brief Write a sequence of optimal nops to the output, covering \p Count
  /// bytes.
  /// \return - true on success, false on failure
  bool writeNopData(raw_ostream &OS, uint64_t Count) const override;
};
} // end anonymous namespace

/// cc—Carry clear      GE—Greater than or equal
/// LS—Lower or same    PL—Plus
/// CS—Carry set        GT—Greater than
/// LT—Less than
/// EQ—Equal            HI—Higher
/// MI—Minus            VC—Overflow clear
///                     LE—Less than or equal
/// NE—Not equal        VS—Overflow set
static unsigned getRelaxedOpcodeBranch(const MCInst &Inst) {
  unsigned Op = Inst.getOpcode();
  switch (Op) {
  default:
    return Op;
  case M680x0::BRA8:
    return M680x0::BRA16;
  case M680x0::Bcc8:
    return M680x0::Bcc16;
  case M680x0::Bls8:
    return M680x0::Bls16;
  case M680x0::Blt8:
    return M680x0::Blt16;
  case M680x0::Beq8:
    return M680x0::Beq16;
  case M680x0::Bmi8:
    return M680x0::Bmi16;
  case M680x0::Bne8:
    return M680x0::Bne16;
  case M680x0::Bge8:
    return M680x0::Bge16;
  case M680x0::Bcs8:
    return M680x0::Bcs16;
  case M680x0::Bpl8:
    return M680x0::Bpl16;
  case M680x0::Bgt8:
    return M680x0::Bgt16;
  case M680x0::Bhi8:
    return M680x0::Bhi16;
  case M680x0::Bvc8:
    return M680x0::Bvc16;
  case M680x0::Ble8:
    return M680x0::Ble16;
  case M680x0::Bvs8:
    return M680x0::Bvs16;
  }
}

static unsigned getRelaxedOpcodeArith(const MCInst &Inst) {
  unsigned Op = Inst.getOpcode();
  switch (Op) {
  default:
    return Op;
    // TODO there will be some relaxations for PCD and ARD mem for x20
  }
}

static unsigned getRelaxedOpcode(const MCInst &Inst) {
  unsigned R = getRelaxedOpcodeArith(Inst);
  if (R != Inst.getOpcode())
    return R;
  return getRelaxedOpcodeBranch(Inst);
}

bool M680x0AsmBackend::mayNeedRelaxation(const MCInst &Inst,
                                         const MCSubtargetInfo &STI) const {
  // Branches can always be relaxed in either mode.
  if (getRelaxedOpcodeBranch(Inst) != Inst.getOpcode())
    return true;

  // Check if this instruction is ever relaxable.
  if (getRelaxedOpcodeArith(Inst) == Inst.getOpcode())
    return false;

  // Check if the relaxable operand has an expression. For the current set of
  // relaxable instructions, the relaxable operand is always the last operand.
  // FIXME will change for x20 mem
  unsigned RelaxableOp = Inst.getNumOperands() - 1;
  if (Inst.getOperand(RelaxableOp).isExpr())
    return true;

  return false;
}

bool M680x0AsmBackend::fixupNeedsRelaxation(const MCFixup &Fixup,
                                            uint64_t Value,
                                            const MCRelaxableFragment *DF,
                                            const MCAsmLayout &Layout) const {
  // TODO Newer CPU can use 32 bit offsets, so check for this when ready
  if (int64_t(Value) != int64_t(int16_t(Value))) {
    llvm_unreachable("Cannot relax the instruction, value does not fit");
  }
  // Relax if the value is too big for a (signed) i8. This means that byte-wide
  // instructions have to matched by default
  //
  //                           NOTE
  // A branch to the immediately following instruction automatically
  // uses the 16-bit displacement format because the 8-bit
  // displacement field contains $00 (zero offset).
  return Value == 0 || int64_t(Value) != int64_t(int8_t(Value));
}

// FIXME: Can tblgen help at all here to verify there aren't other instructions
// we can relax?
void M680x0AsmBackend::relaxInstruction(const MCInst &Inst,
                                        const MCSubtargetInfo &STI,
                                        MCInst &Res) const {
  // The only relaxations M680x0 does is from a 1byte pcrel to a 2byte PCRel.
  unsigned RelaxedOp = getRelaxedOpcode(Inst);

  if (RelaxedOp == Inst.getOpcode()) {
    SmallString<256> Tmp;
    raw_svector_ostream OS(Tmp);
    Inst.dump_pretty(OS);
    OS << "\n";
    report_fatal_error("unexpected instruction to relax: " + OS.str());
  }

  Res = Inst;
  Res.setOpcode(RelaxedOp);
}

bool M680x0AsmBackend::writeNopData(raw_ostream &OS, uint64_t Count) const {
  // Cannot emit NOP with size being not multiple of 16 bits.
  if (Count % 2 != 0)
    return false;

  uint64_t NumNops = Count / 2;
  for (uint64_t i = 0; i != NumNops; ++i) {
    OS << "\x4E\x71";
  }

  return true;
}

namespace {

class M680x0ELFAsmBackend : public M680x0AsmBackend {
public:
  uint8_t OSABI;
  M680x0ELFAsmBackend(const Target &T, uint8_t OSABI)
      : M680x0AsmBackend(T), OSABI(OSABI) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createM680x0ELFObjectWriter(OSABI);
  }
};

} // end anonymous namespace

MCAsmBackend *llvm::createM680x0AsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options) {
  // assert (TheTriple.getEnvironment() == Triple::GNU);
  const Triple &TheTriple = STI.getTargetTriple();
  uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(TheTriple.getOS());
  return new M680x0ELFAsmBackend(T, OSABI);
}
