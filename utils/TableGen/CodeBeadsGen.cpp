//===- CodeBeadsGen.cpp - Code Beads Generator ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// TODO add description
//===----------------------------------------------------------------------===//

#include "CodeGenTarget.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Debug.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"
#include "llvm/TableGen/Error.h"
#include <map>
#include <string>
#include <vector>
using namespace llvm;

namespace {

class CodeBeadsGen {
  RecordKeeper &Records;
public:
  CodeBeadsGen(RecordKeeper &R) : Records(R) {}
  void run(raw_ostream &o);
};

void CodeBeadsGen::run(raw_ostream &o) {
  CodeGenTarget Target(Records);
  std::vector<Record*> Insts = Records.getAllDerivedDefinitions("Instruction");

  // For little-endian instruction bit encodings, reverse the bit order
  Target.reverseBitsForLittleEndianEncoding();

  ArrayRef<const CodeGenInstruction*> NumberedInstructions =
    Target.getInstructionsByEnumValue();

  // Emit function declaration
  o << "const uint64_t * " << Target.getName();
  o << "MCCodeEmitter::getGenInstrBeads(const MCInst &MI,\n"
    << "\tSmallVectorImpl<MCFixup> &Fixups,\n"
    << "\tconst MCSubtargetInfo &STI) const {\n";

  // Emit instruction base values
  // TODO Make it auto-detect size
  o << "  static const uint64_t InstBits[][3] = {\n";
  for (const CodeGenInstruction *CGI : NumberedInstructions) {
    Record *R = CGI->TheDef;

    if (R->getValueAsString("Namespace") == "TargetOpcode" ||
        R->getValueAsBit("isPseudo")) {
      o << "\t{ 0x0, 0x0, 0x0 },";
      o << '\t' << "// " << R->getName() << "\n";
      continue;
    }

    BitsInit *BI = R->getValueAsBitsInit("Beads");

    if (!BI->isComplete()) {
      PrintFatalError(R->getLoc(), "Record `" + R->getName() +
          "', bit field 'Beads' is not complete");
    }

    if (BI->getNumBits() > 192) {
      PrintFatalError(R->getLoc(), "Record `" + R->getName() +
          "', bit field 'Beads' is too long(maximum: 192)");
    }

    unsigned Parts = 3;

    o << "\t{";
    for (unsigned p = 0; p < Parts; ++p) {
      unsigned Num = BI->getNumBits();
      unsigned Right = 64 * p;
      unsigned Left = Right + 64;

      uint64_t Value = 0;
      for (unsigned i = Right; i != Left; ++i) {
        unsigned bit = Num-i-1;
        if (BitInit *B = dyn_cast<BitInit>(BI->getBit(bit))) {
          Value |= (uint64_t)B->getValue() << (bit);
        } else {
          PrintFatalError(R->getLoc(), "Record `" + R->getName() +
              "', bit 'Beads[" + std::to_string(bit) + "]' is not defined");
        }
      }

      if (p) o << ',';
      o << " 0x";
      o.write_hex(Value);
      o << "";
    }
    o << " }," << '\t' << "// " << R->getName() << "\n";

  }
  o << "\t{ 0x0, 0x0, 0x0 }\n  };\n";

  // Emit initial function code
  o << "  const unsigned opcode = MI.getOpcode();\n"
    << "  return InstBits[opcode];\n"
    << "}\n\n";
}

} // End anonymous namespace

namespace llvm {

void EmitCodeBeads(RecordKeeper &RK, raw_ostream &OS) {
  emitSourceFileHeader("Machine Code Beads", OS);
  CodeBeadsGen(RK).run(OS);
}

} // End llvm namespace
