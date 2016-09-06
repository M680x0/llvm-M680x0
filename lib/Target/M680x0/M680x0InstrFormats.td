//=== M680x0InstrFormats.td - Motorola 680x0 Instruction Formats tablegen -===//
// ADDRESS REGISTER INDIRECT WITH DISPLACEMENT. This addressing mode requires one
// word of extension. The address of the operand is the sum of the address in
// the address register and the sign-extended 16-bit displacement integer in the
// extension word. The reference is classified as a data reference with the
// exception of the jump and jump-to-subroutine instructions.
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// M680x0 INSTRUCTION. Most instructions specify the location of an operand by
// using the effective address field in the operation word. The effective address
// is composed of two 3-bit fields: the mode field and the register field. The
// value in the mode field selects the different address modes. The register
// field contains the number of a register.  The effective address field may
// require additional information to fully specify the operand. This additional
// information, called the effective address extension, is contained in the
// following word or words and is considered part of the instruction. The
// effective address modes are grouped into three categories: register direct,
// memory addressing, and special.
class M680x0Inst<bits<16> opcode,
              dag outs, dag ins,
              string asmStr,
              list<dag> pattern,
              InstrItinClass itin = NoItinerary>
    : Instruction {
  let Namespace      = "M680x0";
  let OutOperandList = outs;
  let InOperandList  = ins;
  let AsmString      = asmStr;
  let Pattern        = pattern;
  let Itinerary      = itin;

  // NOTE Opcode is no really applicable for the most instructions since they are
  // composed based on the actual register used and EA mode
  bits<16> Opcode = opcode;
}
