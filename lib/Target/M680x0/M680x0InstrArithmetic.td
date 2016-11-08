//===-- M680x0InstrArithmetic.td - Integer Arithmetic Instrs - tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the integer arithmetic instructions in the M680x0
// architecture. Here is the current status of the file:
//
//  Machine:
//
//    ADD       [~]   ADDA      [ ]   ADDI        [ ]   ADDQ [ ]   ADDX [ ]
//    CLR       [ ]   CMP       [ ]   CMPA        [ ]   CMPI [ ]   CMPM [ ]
//    CMP2      [ ]   DIVS/DIVU [x]   DIVSL/DIVUL [ ]   EXT  [x]   EXTB [ ]
//    MULS/MULU [ ]   NEG       [ ]   NEGX        [ ]   SUB  [ ]   SUBA [ ]
//    SUBI      [ ]   SUBQ      [ ]   SUBX        [ ]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - in progress but usable
//   [x] - done
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Encoding
//===----------------------------------------------------------------------===//

/// Encoding for Normal forms
/// ----------------------------------------------------
///  F  E  D  C | B  A  9 | 8  7  6 | 5  4  3 | 2  1  0
/// ----------------------------------------------------
///             |         |         | EFFECTIVE ADDRESS
///  x  x  x  x |   REG   | OP MODE |   MODE  |   REG
/// ----------------------------------------------------
class MxArithEncoding<MxBead4Bits CMD, MxEncOpMode OPMODE, MxBeadReg REG,
                      MxEncEA EA, MxEncExt EXT>
    : MxEncoding<EA.Reg, EA.DA, EA.Mode, OPMODE.B0, OPMODE.B1, OPMODE.B2, REG, CMD,
      EXT.Imm, EXT.B8, EXT.Scale, EXT.WL, EXT.DAReg>;

/// Encoding for Immediate forms
/// ---------------------------------------------------
///  F  E  D  C  B  A  9  8 | 7  6 | 5  4  3 | 2  1  0
/// ---------------------------------------------------
///                         |      | EFFECTIVE ADDRESS
///  x  x  x  x  x  x  x  x | SIZE |   MODE  |   REG
/// ---------------------------------------------------
///     16-BIT WORD DATA    |     8-BIT BYTE DATA
/// ---------------------------------------------------
///                 32-BIT LONG DATA
/// ---------------------------------------------------
class MxArithImmEncoding<MxBead4Bits CMD, MxEncSize SIZE,
                         MxEncEA DST_EA, MxEncExt DST_EXT, MxEncExt SRC_EXT>
    : MxEncoding<DST_EA.Reg, DST_EA.DA, DST_EA.Mode, SIZE, CMD, MxBead4Bits<0>,
      SRC_EXT.Imm, SRC_EXT.B8, SRC_EXT.Scale, SRC_EXT.WL, SRC_EXT.DAReg,
      DST_EXT.Imm, DST_EXT.B8, DST_EXT.Scale, DST_EXT.WL, DST_EXT.DAReg>;

/* class MxImmEncoding<MxEncEA EA, MxOpMode OPMODE, bits<4> CMD, MxEncExt EXT> */
/*     : MxEncoding<EA, OPMODE, MxBeadReg<>>; */

//===----------------------------------------------------------------------===//
// Add/Sub
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxBiArOp_RFRR<string MN, SDNode NODE, MxOperand DST, MxOperand OPD>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           MN#"."#DST.Size.Id#"\t$opd, $dst",
           [(set DST.VT:$dst, CCR, (NODE DST.VT:$src, OPD.VT:$opd))]>;

// $reg <- $reg op $imm
class MxBiArOp_RFRI<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxIIII<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.IOp:$opd),
              MN#"."#TYPE.Prefix#"\t$opd, $dst",
              [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.IPat:$opd))],
              MxArithImmEncoding<MxBead4Bits<CMD>, !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                   !cast<MxEncEA>("MxEncEA"#TYPE.RLet#"_0"), MxExtEmpty,
                   !cast<MxEncExt>("MxExtI"#TYPE.Size#"_2")>>;

class MxBiArOp_RFRM<string MN, SDNode NODE, MxType TYPE, MxOperand OPD, ComplexPattern PAT,
                    bits<4> CMD, MxEncEA EA, MxEncExt EXT>
    : MxIIII<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, OPD:$opd),
           MN#"."#TYPE.Prefix#"\t$opd, $dst",
           [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, (TYPE.Load PAT:$opd)))],
           MxArithEncoding<MxBead4Bits<CMD>,
                           !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#TYPE.RLet#"EA"),
                           MxBeadReg<0>, EA, EXT>>;


// Allows use of An registers
multiclass MxBiArOp_RF<string MN, SDNode MXNODE, bits<4> CMD, bits<4> CMDI> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = 1 in {

def NAME#"8dd"  : MxBiArOp_RFRR<MN, MXNODE, MxDRD8,  MxDRD8>;
def NAME#"16dd" : MxBiArOp_RFRR<MN, MXNODE, MxXRD16, MxXRD16>;
def NAME#"32rr" : MxBiArOp_RFRR<MN, MXNODE, MxXRD32, MxXRD32>;

} // isCommutable = 1

def NAME#"8dj"  : MxBiArOp_RFRM<MN, MXNODE, MxType8d,  MxType8.JOp,  MxType8.JPat,  CMD, MxEncEAj_2, MxExtEmpty>;
def NAME#"16dj" : MxBiArOp_RFRM<MN, MXNODE, MxType16d, MxType16.JOp, MxType16.JPat, CMD, MxEncEAj_2, MxExtEmpty>;
def NAME#"32rj" : MxBiArOp_RFRM<MN, MXNODE, MxType32r, MxType32.JOp, MxType32.JPat, CMD, MxEncEAj_2, MxExtEmpty>;

def NAME#"8dp"  : MxBiArOp_RFRM<MN, MXNODE, MxType8d,  MxType8.POp,  MxType8.PPat,  CMD, MxEncEAp_2, MxExtI16_2>;
def NAME#"16dp" : MxBiArOp_RFRM<MN, MXNODE, MxType16d, MxType16.POp, MxType16.PPat, CMD, MxEncEAp_2, MxExtI16_2>;
def NAME#"32rp" : MxBiArOp_RFRM<MN, MXNODE, MxType32r, MxType32.POp, MxType32.PPat, CMD, MxEncEAp_2, MxExtI16_2>;

def NAME#"8df"  : MxBiArOp_RFRM<MN, MXNODE, MxType8d,  MxType8.FOp,  MxType8.FPat,  CMD, MxEncEAf_2, MxExtBrief_2>;
def NAME#"16df" : MxBiArOp_RFRM<MN, MXNODE, MxType16d, MxType16.FOp, MxType16.FPat, CMD, MxEncEAf_2, MxExtBrief_2>;
def NAME#"32rf" : MxBiArOp_RFRM<MN, MXNODE, MxType32r, MxType32.FOp, MxType32.FPat, CMD, MxEncEAf_2, MxExtBrief_2>;

def NAME#"8di"  : MxBiArOp_RFRI<MN, MXNODE, MxType8d,  CMDI>;
def NAME#"16di" : MxBiArOp_RFRI<MN, MXNODE, MxType16d, CMDI>;
def NAME#"32ri" : MxBiArOp_RFRI<MN, MXNODE, MxType32r, CMDI>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_RF

defm ADD : MxBiArOp_RF<"add", MxAdd, 0xD, 0x6>;
defm SUB : MxBiArOp_RF<"sub", MxSub, 0x9, 0x4>;

//===----------------------------------------------------------------------===//
// And/Xor/Or
//===----------------------------------------------------------------------===//

// Does not allow use of An registers
multiclass MxBiArOp_DF<string MN, SDNode MXNODE, SDNode NODE, bits<4> CMDI> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = 1 in {

def NAME#"8dd"  : MxBiArOp_RFRR<MN, MXNODE, MxDRD8,  MxDRD8>;
def NAME#"16dd" : MxBiArOp_RFRR<MN, MXNODE, MxDRD16, MxDRD16>;
def NAME#"32dd" : MxBiArOp_RFRR<MN, MXNODE, MxDRD32, MxDRD32>;

} // isCommutable = 1

def NAME#"8di"  : MxBiArOp_RFRI<MN, MXNODE,  MxType8d, CMDI>;
def NAME#"16di" : MxBiArOp_RFRI<MN, MXNODE, MxType16d, CMDI>;
def NAME#"32di" : MxBiArOp_RFRI<MN, MXNODE, MxType32d, CMDI>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_RF

defm AND : MxBiArOp_DF<"and", MxAnd, and, 0x2>;
defm XOR : MxBiArOp_DF<"eor", MxXor, xor, 0xA>;
defm OR  : MxBiArOp_DF<"or",  MxOr,   or, 0x0>;

//===----------------------------------------------------------------------===//
// Cmp
//===----------------------------------------------------------------------===//

class MxCmp<dag ins, string symbol = "cmp">
    : MxInst<0x0, (outs CCRC:$ccr), ins, symbol#"\t$lhs, $rhs">;

def CMP8dd  : MxCmp<(ins MxDRD8 :$lhs, MxDRD8 :$rhs)>;
def CMP16dd : MxCmp<(ins MxDRD16:$lhs, MxDRD16:$rhs)>;
def CMP32dd : MxCmp<(ins MxDRD32:$lhs, MxDRD32:$rhs)>;

let mayLoad = 1 in {
def CMP8dj  : MxCmp<(ins MxARI8 :$lhs, MxDRD8 :$rhs)>;
def CMP16dj : MxCmp<(ins MxARI16:$lhs, MxDRD16:$rhs)>;
def CMP32dj : MxCmp<(ins MxARI32:$lhs, MxDRD32:$rhs)>;

def CMP8dp  : MxCmp<(ins MxARID8 :$lhs, MxDRD8 :$rhs)>;
def CMP16dp : MxCmp<(ins MxARID16:$lhs, MxDRD16:$rhs)>;
def CMP32dp : MxCmp<(ins MxARID32:$lhs, MxDRD32:$rhs)>;

def CMP8df  : MxCmp<(ins MxARII8 :$lhs, MxDRD8 :$rhs)>;
def CMP16df : MxCmp<(ins MxARII16:$lhs, MxDRD16:$rhs)>;
def CMP32df : MxCmp<(ins MxARII32:$lhs, MxDRD32:$rhs)>;
}

def CMP8di  : MxCmp<(ins MxDRD8 :$lhs, Mxi8imm :$rhs), "cmpi">;
def CMP16di : MxCmp<(ins MxDRD16:$lhs, Mxi16imm:$rhs), "cmpi">;
def CMP32di : MxCmp<(ins MxDRD32:$lhs, Mxi32imm:$rhs), "cmpi">;

//===----------------------------------------------------------------------===//
// Ext
//===----------------------------------------------------------------------===//

let Constraints = "$src = $dst" in {
  def EXT16 : MxInst<0x0, (outs MxDRD16:$dst), (ins MxDRD16:$src), "ext.w\t$src">;
  def EXT32 : MxInst<0x0, (outs MxDRD32:$dst), (ins MxDRD32:$src), "ext.l\t$src">;
} // $src = $dst

def : Pat<(sext_inreg i16:$src, i8),  (EXT16 $src)>;
def : Pat<(sext_inreg i32:$src, i16), (EXT32 $src)>;
/* def : Pat<(sext_inreg i32:$src, i8),  (EXT32 (EXT16 $src))>; */

//===----------------------------------------------------------------------===//
// Div/Mul
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxDiMuOp_DD<string MN, MxOperand DST, MxOperand OPD>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           MN#"\t$opd, $dst", []>;

// $reg <- $reg op $imm
class MxDiMuOp_DI<string MN, MxOperand DST, MxOperand OPD>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           MN#"\t$opd, $dst", []>;

multiclass MxDiMuOp<string MN> {

let Defs = [CCR] in {
  def "S"#NAME#"d32d16" : MxDiMuOp_DD<MN#"s", MxDRD32, MxDRD16>;
  def "U"#NAME#"d32d16" : MxDiMuOp_DD<MN#"u", MxDRD32, MxDRD16>;

  def "S"#NAME#"d32i16" : MxDiMuOp_DI<MN#"s", MxDRD32, Mxi16imm>;
  def "U"#NAME#"d32i16" : MxDiMuOp_DI<MN#"u", MxDRD32, Mxi16imm>;
} // Defs = [CCR]

}

// TODO immediates
let Constraints = "$src = $dst" in
defm DIV : MxDiMuOp<"div">;

def : Pat<(sdiv i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (SDIVd32d16 (MOVSXd32d16 $dst), $opd),
            MxSubRegIndex16Lo)>;

def : Pat<(udiv i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (UDIVd32d16 (MOVZXd32d16 $dst), $opd),
            MxSubRegIndex16Lo)>;

def : Pat<(srem i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (ASR32di (ASR32di (SDIVd32d16 (MOVSXd32d16 $dst), $opd), 8), 8),
            MxSubRegIndex16Lo)>;

def : Pat<(urem i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (LSR32di (LSR32di (UDIVd32d16 (MOVZXd32d16 $dst), $opd), 8), 8),
            MxSubRegIndex16Lo)>;


let isCommutable = 1 in
defm MUL : MxDiMuOp<"mul">;

def : Pat<(mul i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (SMULd32d16 (MOVXd32d16 $dst), $opd),
            MxSubRegIndex16Lo)>;

def : Pat<(mulhs i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (ASR32di (ASR32di (SMULd32d16 (MOVXd32d16 $dst), $opd), 8), 8),
            MxSubRegIndex16Lo)>;

def : Pat<(mulhu i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (LSR32di (LSR32di (UMULd32d16 (MOVXd32d16 $dst), $opd), 8), 8),
            MxSubRegIndex16Lo)>;

//===----------------------------------------------------------------------===//
// no-CCR Patterns
//===----------------------------------------------------------------------===//

// TODO figure out why the fuck i cannot use Mxi8/16/32imm as operand,
// but only imm.

// add reg, reg
def : Pat<(add i8 :$src, i8 :$opd), (ADD8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(add i16:$src, i16:$opd), (ADD16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(add i32:$src, i32:$opd), (ADD32rr MxXRD32:$src, MxXRD32:$opd)>;

// add (An), reg
def : Pat<(add MxType8.VT:$src, (Mxloadi8 MxType8.JPat:$opd)),
          (ADD8dj MxDRD8:$src, MxType8.JOp:$opd)>;
def : Pat<(add MxType16.VT:$src, (Mxloadi16 MxType16.JPat:$opd)),
          (ADD16dj MxDRD16:$src, MxType16.JOp:$opd)>;
def : Pat<(add MxType32.VT:$src, (Mxloadi32 MxType32.JPat:$opd)),
          (ADD32rj MxXRD32:$src, MxType32.JOp:$opd)>;

// add (i,An), reg
def : Pat<(add MxType8.VT:$src, (Mxloadi8 MxType8.PPat:$opd)),
          (ADD8dp MxDRD8:$src, MxType8.POp:$opd)>;
def : Pat<(add MxType16.VT:$src, (Mxloadi16 MxType16.PPat:$opd)),
          (ADD16dp MxDRD16:$src, MxType16.POp:$opd)>;
def : Pat<(add MxType32.VT:$src, (Mxloadi32 MxType32.PPat:$opd)),
          (ADD32rp MxXRD32:$src, MxType32.POp:$opd)>;

// add (i,An,Xn), reg
def : Pat<(add MxType8.VT:$src, (Mxloadi8 MxType8.FPat:$opd)),
          (ADD8df MxDRD8:$src, MxType8.FOp:$opd)>;
def : Pat<(add MxType16.VT:$src, (Mxloadi16 MxType16.FPat:$opd)),
          (ADD16df MxDRD16:$src, MxType16.FOp:$opd)>;
def : Pat<(add MxType32.VT:$src, (Mxloadi32 MxType32.FPat:$opd)),
          (ADD32rf MxXRD32:$src, MxType32.FOp:$opd)>;

// add reg, imm
def : Pat<(add i8: $src, MximmSExt8:$opd) , (ADD8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(add i16:$src, MximmSExt16:$opd), (ADD16di MxDRD16:$src, imm:$opd)>;
def : Pat<(add i32:$src, MximmSExt32:$opd), (ADD32ri MxXRD32:$src, imm:$opd)>;

// TODO checkout if i need a similar patters and for sub too
/* def : Pat<(add MxXRD16:$src, i16immSExt8:$opd), */
/*           (ADD16ri8 MxXRD16:$src, i16immSExt8:$opd)>; */
/* def : Pat<(add MxXRD32:$src, i32immSExt8:$opd), */
/*           (ADD32ri8 MxXRD32:$src, i32immSExt8:$opd)>; */

// sub reg, reg
def : Pat<(sub i8 :$src, i8 :$opd), (SUB8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(sub i16:$src, i16:$opd), (SUB16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(sub i32:$src, i32:$opd), (SUB32rr MxXRD32:$src, MxXRD32:$opd)>;

/* // sub reg, mem */
/* def : Pat<(sub MxDRD8:$src, (Mxloadi8 addr:$opd)), */
/*           (SUB8rm MxDRD8:$src, addr:$opd)>; */
/* def : Pat<(sub MxXRD16:$src, (Mxloadi16 addr:$opd)), */
/*           (SUB16rm MxXRD16:$src, addr:$opd)>; */
/* def : Pat<(sub MxXRD32:$src, (Mxloadi32 addr:$opd)), */
/*           (SUB32rm MxXRD32:$src, addr:$opd)>; */

// sub reg, imm
def : Pat<(sub i8 :$src, MximmSExt8 :$opd), (SUB8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(sub i16:$src, MximmSExt16:$opd), (SUB16di MxDRD16:$src, imm:$opd)>;
def : Pat<(sub i32:$src, MximmSExt32:$opd), (SUB32ri MxXRD32:$src, imm:$opd)>;

// and reg, reg
def : Pat<(and i8 :$src, i8 :$opd), (AND8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(and i16:$src, i16:$opd), (AND16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(and i32:$src, i32:$opd), (AND32dd MxDRD32:$src, MxDRD32:$opd)>;

// and reg, mem
/* def : Pat<(and MxDRD8:$src, (Mxloadi8 addr:$opd)), */
/*           (AND8rm MxDRD8:$src, addr:$opd)>; */
/* def : Pat<(and MxXRD16:$src, (Mxloadi16 addr:$opd)), */
/*           (AND16rm MxXRD16:$src, addr:$opd)>; */
/* def : Pat<(and MxXRD32:$src, (Mxloadi32 addr:$opd)), */
/*           (AND32rm MxXRD32:$src, addr:$opd)>; */
/* def : Pat<(and GR64:$src, (loadi64 addr:$opd)), */
/*           (AND64rm GR64:$src, addr:$opd)>; */

// and reg, imm
def : Pat<(and i8: $src, MximmSExt8 :$opd), (AND8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(and i16:$src, MximmSExt16:$opd), (AND16di MxDRD16:$src, imm:$opd)>;
def : Pat<(and i32:$src, MximmSExt32:$opd), (AND32di MxDRD32:$src, imm:$opd)>;

// xor reg,reg
def : Pat<(xor i8 :$src, i8 :$opd), (XOR8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(xor i16:$src, i16:$opd), (XOR16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(xor i32:$src, i32:$opd), (XOR32dd MxDRD32:$src, MxDRD32:$opd)>;

// xor reg, mem
/* def : Pat<(xor MxDRD8:$src, (Mxloadi8 addr:$opd)), */
/*           (XOR8rm MxDRD8:$src, addr:$opd)>; */
/* def : Pat<(xor MxXRD16:$src, (Mxloadi16 addr:$opd)), */
/*           (XOR16rm MxXRD16:$src, addr:$opd)>; */
/* def : Pat<(xor MxXRD32:$src, (Mxloadi32 addr:$opd)), */
/*           (XOR32rm MxXRD32:$src, addr:$opd)>; */
/* def : Pat<(xor GR64:$src, (loadi64 addr:$opd)), */
/*           (XOR64rm GR64:$src, addr:$opd)>; */

// xor reg, imm
def : Pat<(xor i8: $src, MximmSExt8 :$opd), (XOR8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(xor i16:$src, MximmSExt16:$opd), (XOR16di MxDRD16:$src, imm:$opd)>;
def : Pat<(xor i32:$src, MximmSExt32:$opd), (XOR32di MxDRD32:$src, imm:$opd)>;

// or reg, reg
def : Pat<(or i8 :$src, i8 :$opd), (OR8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(or i16:$src, i16:$opd), (OR16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(or i32:$src, i32:$opd), (OR32dd MxDRD32:$src, MxDRD32:$opd)>;

// or reg/mem
/* def : Pat<(or MxDRD8:$src, (Mxloadi8 addr:$opd)), */
/*           (OR8rm MxDRD8:$src, addr:$opd)>; */
/* def : Pat<(or MxXRD16:$src, (Mxloadi16 addr:$opd)), */
/*           (OR16rm MxXRD16:$src, addr:$opd)>; */
/* def : Pat<(or MxXRD32:$src, (Mxloadi32 addr:$opd)), */
/*           (OR32rm MxXRD32:$src, addr:$opd)>; */
/* def : Pat<(or GR64:$src, (loadi64 addr:$opd)), */
/*           (OR64rm GR64:$src, addr:$opd)>; */

// or reg, imm
def : Pat<(or i8: $src, MximmSExt8 :$opd), (OR8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(or i16:$src, MximmSExt16:$opd), (OR16di MxDRD16:$src, imm:$opd)>;
def : Pat<(or i32:$src, MximmSExt32:$opd), (OR32di MxDRD32:$src, imm:$opd)>;
