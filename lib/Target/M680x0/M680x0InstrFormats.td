//=== M680x0InstrFormats.td - Motorola 680x0 Instruction Formats tablegen -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Since M680x0 has quite a lot memory addressing modes there are more
// instruction prefixes than just i, r and m:
// TSF  Since     Form                     Letter  Description
//  00   M68000    Dn or An                 r       any register
//  01   M68000    Dn                       d       data register direct
//  02   M68000    An                       a       address register direct
//  03   M68000    (An)                     j       address register indirect
//  04   M68000    (An)+                    o       address register indirect with postincrement
//  05   M68000    -(An)                    e       address register indirect with predecrement
//  06   M68000    (i,An)                   p       address register indirect with displacement
//  10   M68000    (i,An,Xn.L)              f       address register indirect with index and scale = 1
//  07   M68000    (i,An,Xn.W)              F       address register indirect with index and scale = 1
//  12   M68020    (i,An,Xn.L,SCALE)        g       address register indirect with index
//  11   M68020    (i,An,Xn.W,SCALE)        G       address register indirect with index
//  14   M68020    ([bd,An],Xn.L,SCALE,od)  u       memory indirect postindexed mode
//  13   M68020    ([bd,An],Xn.W,SCALE,od)  U       memory indirect postindexed mode
//  16   M68020    ([bd,An,Xn.L,SCALE],od)  v       memory indirect preindexed mode
//  15   M68020    ([bd,An,Xn.W,SCALE],od)  V       memory indirect preindexed mode
//  20   M68000    abs.L                    b       absolute long address
//  17   M68000    abs.W                    B       absolute short address
//  21   M68000    (i,PC)                   q       program counter with displacement
//  23   M68000    (i,PC,Xn.L)              k       program counter with index and scale = 1
//  22   M68000    (i,PC,Xn.W)              K       program counter with index and scale = 1
//  25   M68020    (i,PC,Xn.L,SCALE)        l       program counter with index
//  24   M68020    (i,PC,Xn.W,SCALE)        L       program counter with index
//  27   M68020    ([bd,PC],Xn.L,SCALE,od)  x       program counter memory indirect postindexed mode
//  26   M68020    ([bd,PC],Xn.W,SCALE,od)  X       program counter memory indirect postindexed mode
//  31   M68020    ([bd,PC,Xn.L,SCALE],od)  y       program counter memory indirect preindexed mode
//  30   M68020    ([bd,PC,Xn.W,SCALE],od)  Y       program counter memory indirect preindexed mode
//  32   M68000    #immediate               i       immediate data
//
// NOTE that long form is always lowercase, word variants are capitalized
//
// Operand can be quilified with size where appropriate to force a particular
// instruction encoding, e.g.:
//    (i8,An,Xn.W)             f8      1 extension word
//    (i16,An,Xn.W)            f16     2 extension words
//    (i32,An,Xn.W)            f32     3 extension words
//
// Form without size qualifier will adapt to operand size automatically, e.g.:
//    (i,An,Xn.W)              f       1, 2 or 3 extension words
//
// Some forms alreay imply a particular size of their operands, e.g.:
//    (i,An)                   p       1 extension word and i is 16bit
//
// Operand order follows x86 Intel order(destination before source), e.g.:
//    MOV8df                   MOVE (4,A0,D0), D1
//
// Number after instruction mnemonics determines the size of the data
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
class MxInst<bits<16> opcode,
             dag outs, dag ins,
             string asmStr = "",
             list<dag> pattern = [],
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

//===----------------------------------------------------------------------===//
// Encoding primitives
//===----------------------------------------------------------------------===//

class MxBead<bits<4> type, bits<4> value> {
  bits<8> Value = 0b00000000;
  let Value{3-0} = type;
  let Value{7-4} = value;
}

def   MxBeadTerm                  : MxBead<0b0000, 0b0000>;
class MxBead1Bit  <bits<1> value> : MxBead<0b0001, 0> { let Value{4}   = value; }
class MxBead2Bits <bits<2> value> : MxBead<0b0010, 0> { let Value{5-4} = value; }
class MxBead3Bits <bits<3> value> : MxBead<0b0011, 0> { let Value{6-4} = value; }
class MxBead4Bits <bits<4> value> : MxBead<0b0100, value>;
class MxBeadDAReg <bits<4> opN>   : MxBead<0b0101, opN>;
class MxBeadReg   <bits<4> opN>   : MxBead<0b0110, opN>;
class MxBead8Imm  <bits<4> opN>   : MxBead<0b0111, opN>;
class MxBead16Imm <bits<4> opN>   : MxBead<0b1000, opN>;
class MxBead32Imm <bits<4> opN>   : MxBead<0b1001, opN>;
def   MxBeadIgnore                : MxBead<0b1111, 0b1111>;

class MxEncoding<MxBead n0  = MxBeadTerm, MxBead n1  = MxBeadTerm, 
                 MxBead n2  = MxBeadTerm, MxBead n3  = MxBeadTerm,
                 MxBead n4  = MxBeadTerm, MxBead n5  = MxBeadTerm,
                 MxBead n6  = MxBeadTerm, MxBead n7  = MxBeadTerm,
                 MxBead n8  = MxBeadTerm, MxBead n9  = MxBeadTerm,
                 MxBead n10 = MxBeadTerm, MxBead n11 = MxBeadTerm,
                 MxBead n12 = MxBeadTerm, MxBead n13 = MxBeadTerm,
                 MxBead n14 = MxBeadTerm, MxBead n15 = MxBeadTerm,
                 MxBead n16 = MxBeadTerm, MxBead n17 = MxBeadTerm,
                 MxBead n18 = MxBeadTerm, MxBead n19 = MxBeadTerm> {
  bits <160> Value;
  let Value{7-0}     = n0.Value;
  let Value{15-8}    = n1.Value;
  let Value{23-16}   = n2.Value;
  let Value{31-24}   = n3.Value;
  let Value{39-32}   = n4.Value;
  let Value{47-40}   = n5.Value;
  let Value{55-48}   = n6.Value;
  let Value{63-56}   = n7.Value;
  let Value{71-64}   = n8.Value;
  let Value{79-72}   = n9.Value;
  let Value{87-80}   = n10.Value;
  let Value{95-88}   = n11.Value;
  let Value{103-96}  = n12.Value;
  let Value{111-104} = n13.Value;
  let Value{119-112} = n14.Value;
  let Value{127-120} = n15.Value;
  let Value{135-128} = n16.Value;
  let Value{143-136} = n17.Value;
  let Value{151-144} = n18.Value;
  let Value{159-152} = n19.Value;
}

class MxEncFixed<bits<16> value> : MxEncoding {
  let Value{7-0}   = MxBead4Bits<value{3-0}>.Value;
  let Value{15-8}  = MxBead4Bits<value{7-4}>.Value;
  let Value{23-16} = MxBead4Bits<value{11-8}>.Value;
  let Value{31-24} = MxBead4Bits<value{15-12}>.Value;
}

def MxEncEmpty : MxEncoding;


class MxEncEA<MxBead mode, MxBead reg> {
  MxBead Mode = mode;
  MxBead Reg = reg;
}

class MxEncEAr<bits<4> opN> : MxEncEA<MxBead2Bits<0b00>,  MxBeadDAReg<opN>>;
class MxEncEAd<bits<4> opN> : MxEncEA<MxBead3Bits<0b000>, MxBeadReg<opN>>;
class MxEncEAa<bits<4> opN> : MxEncEA<MxBead3Bits<0b001>, MxBeadReg<opN>>;
class MxEncEAj<bits<4> opN> : MxEncEA<MxBead3Bits<0b010>, MxBeadReg<opN>>;
class MxEncEAo<bits<4> opN> : MxEncEA<MxBead3Bits<0b011>, MxBeadReg<opN>>;
class MxEncEAe<bits<4> opN> : MxEncEA<MxBead3Bits<0b100>, MxBeadReg<opN>>;
class MxEncEAp<bits<4> opN> : MxEncEA<MxBead3Bits<0b101>, MxBeadReg<opN>>;
class MxEncEAf<bits<4> opN> : MxEncEA<MxBead3Bits<0b110>, MxBeadReg<opN>>;
class MxEncEAb<bits<4> opN> : MxEncEA<MxBead3Bits<0b111>, MxBead3Bits<0b000>>;
class MxEncEAB<bits<4> opN> : MxEncEA<MxBead3Bits<0b111>, MxBead3Bits<0b001>>;
class MxEncEAq<bits<4> opN> : MxEncEA<MxBead3Bits<0b111>, MxBead3Bits<0b010>>;
class MxEncEAg<bits<4> opN> : MxEncEA<MxBead3Bits<0b111>, MxBead3Bits<0b011>>;
class MxEncEAi<bits<4> opN> : MxEncEA<MxBead3Bits<0b111>, MxBead3Bits<0b100>>;


// FIXME rename to MxInst
class MxInstruction<dag outs, dag ins,
             string asmStr = "",
             list<dag> pattern = [],
             MxEncoding nimbles = MxEncEmpty,
             InstrItinClass itin = NoItinerary>
    : Instruction {
  let Namespace      = "M680x0";
  let OutOperandList = outs;
  let InOperandList  = ins;
  let AsmString      = asmStr;
  let Pattern        = pattern;
  let Itinerary      = itin;

  // Byte stream
  field bits<160> Inst = nimbles.Value;

  // Number of bytes
  let Size = 0;
}

// M680x0 PSEUDO INSTRUCTION
class MxPseudo<dag outs, dag ins, string asmStr = "", list<dag> pattern = []>
    : MxInst<0x0, outs, ins, asmStr, pattern> {
  let isPseudo = 1;
}
