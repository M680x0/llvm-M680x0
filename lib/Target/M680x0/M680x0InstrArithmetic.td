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
//    ADD       [~]   ADDA      [~]   ADDI        [~]   ADDQ [ ]   ADDX [~]
//    CLR       [ ]   CMP       [~]   CMPA        [~]   CMPI [~]   CMPM [ ]
//    CMP2      [ ]   DIVS/DIVU [~]   DIVSL/DIVUL [ ]   EXT  [~]   EXTB [ ]
//    MULS/MULU [~]   NEG       [~]   NEGX        [ ]   SUB  [~]   SUBA [~]
//    SUBI      [~]   SUBQ      [ ]   SUBX        [~]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - functional implementation
//   [X] - complete implementation
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

/// Encoding for Extended forms
/// ------------------------------------------------------
///  F  E  D  C | B  A  9 | 8 | 7  6 | 5  4 | 3 | 2  1  0
/// ------------------------------------------------------
///  x  x  x  x |  REG Rx | 1 | SIZE | 0  0 | M |  REG Ry
/// ------------------------------------------------------
/// Rx - destination
/// Ry - source
/// M  - address mode switch
class MxArithXEncoding<MxBead4Bits CMD, MxEncSize SIZE, MxBead1Bit MODE,
                       MxBeadReg SRC, MxBeadReg DST>
    : MxEncoding<SRC, MODE, MxBead2Bits<0b00>, SIZE, MxBead1Bit<0b1>, DST, CMD>;

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
/// NOTE It is used to store an immediate to memory, imm-to-reg are handled with
/// normal version
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

// $reg, $ccr <- $reg op $reg
class MxBiArOp_RFRR_xEA<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.ROp:$opd),
           MN#"."#TYPE.Prefix#"\t$opd, $dst",
           [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.VT:$opd))],
           MxArithEncoding<MxBead4Bits<CMD>,
                           !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#TYPE.RLet#"EA"),
                           MxBeadReg<0>, !cast<MxEncEA>("MxEncEA"#TYPE.RLet#"_2"), MxExtEmpty>>;

/// This Op is similar to the one above except it uses reversed opmode, some
/// commands(e.g. eor) do not support dEA or rEA modes and require EAd for
/// register only operations.
/// NOTE when using dd commands it is irrelevant which opmode to use(as it seems)
/// but some opcodes support address register and some do not which creates this
/// mess.
class MxBiArOp_RFRR_EAd<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.ROp:$opd),
           MN#"."#TYPE.Prefix#"\t$opd, $dst",
           [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.VT:$opd))],
           MxArithEncoding<MxBead4Bits<CMD>,
                           !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#"EAd"),
                           MxBeadReg<2>, MxEncEAd_0, MxExtEmpty>>;

// $reg <- $reg op $imm
class MxBiArOp_RFRI_xEA<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.IOp:$opd),
              MN#"."#TYPE.Prefix#"\t$opd, $dst",
              [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.IPat:$opd))],
              MxArithEncoding<MxBead4Bits<CMD>,
                              !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#TYPE.RLet#"EA"),
                              MxBeadReg<0>, MxEncEAi,
                              !cast<MxEncExt>("MxExtI"#TYPE.Size#"_2")>>;

// Again, there are two ways to write an immediate to Dn register either dEA
// opmode or using *I encoding, and again some instrucitons also support address
// registers some do not.
class MxBiArOp_RFRI<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.IOp:$opd),
              MN#"i."#TYPE.Prefix#"\t$opd, $dst",
              [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.IPat:$opd))],
              MxArithImmEncoding<MxBead4Bits<CMD>, !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                   !cast<MxEncEA>("MxEncEA"#TYPE.RLet#"_0"), MxExtEmpty,
                   !cast<MxEncExt>("MxExtI"#TYPE.Size#"_2")>>;

let mayLoad = 1 in
let mayStore = 1 in
class MxBiArOp_RFMI<string MN, SDNode NODE, MxType TYPE,
                    MxOperand MEMOpd, ComplexPattern MEMPat,
                    bits<4> CMD, MxEncEA MEMEA, MxEncExt MEMExt>
    : MxInst<(outs), (ins MEMOpd:$dst, TYPE.IOp:$opd),
              MN#"."#TYPE.Prefix#"\t$opd, $dst",
              [(store (NODE (load MEMPat:$dst), TYPE.IPat:$opd), MEMPat:$dst)],
              MxArithImmEncoding<MxBead4Bits<CMD>,
                                 !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                                 MEMEA, MEMExt,
                                 !cast<MxEncExt>("MxExtI"#TYPE.Size#"_1")>>;

class MxBiArOp_RFRM<string MN, SDNode NODE, MxType TYPE, MxOperand OPD, ComplexPattern PAT,
                    bits<4> CMD, MxEncEA EA, MxEncExt EXT>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, OPD:$opd),
           MN#"."#TYPE.Prefix#"\t$opd, $dst",
           [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, (TYPE.Load PAT:$opd)))],
           MxArithEncoding<MxBead4Bits<CMD>,
                           !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#TYPE.RLet#"EA"),
                           MxBeadReg<0>, EA, EXT>>;


multiclass MxBiArOp_RF<string MN, SDNode NODE, bit isComm, bits<4> CMD, bits<4> CMDI> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = isComm in {

def NAME#"8dd"  : MxBiArOp_RFRR_xEA<MN, NODE, MxType8d,  CMD>;
def NAME#"16dd" : MxBiArOp_RFRR_xEA<MN, NODE, MxType16d, CMD>;
def NAME#"32rr" : MxBiArOp_RFRR_xEA<MN, NODE, MxType32r, CMD>;

} // isCommutable = ?

def NAME#"8dj"  : MxBiArOp_RFRM<MN, NODE, MxType8d,  MxType8.JOp,  MxType8.JPat,  CMD, MxEncEAj_2, MxExtEmpty>;
def NAME#"16dj" : MxBiArOp_RFRM<MN, NODE, MxType16d, MxType16.JOp, MxType16.JPat, CMD, MxEncEAj_2, MxExtEmpty>;
def NAME#"32rj" : MxBiArOp_RFRM<MN, NODE, MxType32r, MxType32.JOp, MxType32.JPat, CMD, MxEncEAj_2, MxExtEmpty>;

def NAME#"8dp"  : MxBiArOp_RFRM<MN, NODE, MxType8d,  MxType8.POp,  MxType8.PPat,  CMD, MxEncEAp_2, MxExtI16_2>;
def NAME#"16dp" : MxBiArOp_RFRM<MN, NODE, MxType16d, MxType16.POp, MxType16.PPat, CMD, MxEncEAp_2, MxExtI16_2>;
def NAME#"32rp" : MxBiArOp_RFRM<MN, NODE, MxType32r, MxType32.POp, MxType32.PPat, CMD, MxEncEAp_2, MxExtI16_2>;

def NAME#"8df"  : MxBiArOp_RFRM<MN, NODE, MxType8d,  MxType8.FOp,  MxType8.FPat,  CMD, MxEncEAf_2, MxExtBrief_2>;
def NAME#"16df" : MxBiArOp_RFRM<MN, NODE, MxType16d, MxType16.FOp, MxType16.FPat, CMD, MxEncEAf_2, MxExtBrief_2>;
def NAME#"32rf" : MxBiArOp_RFRM<MN, NODE, MxType32r, MxType32.FOp, MxType32.FPat, CMD, MxEncEAf_2, MxExtBrief_2>;

def NAME#"8di"  : MxBiArOp_RFRI_xEA<MN, NODE, MxType8d,  CMD>;
def NAME#"16di" : MxBiArOp_RFRI_xEA<MN, NODE, MxType16d, CMD>;
def NAME#"32ri" : MxBiArOp_RFRI_xEA<MN, NODE, MxType32r, CMD>;

} // $src = $dst

def NAME#"8ji"  : MxBiArOp_RFMI<MN, NODE, MxType8,  MxType8.JOp,  MxType8.JPat,  CMD, MxEncEAj_0, MxExtEmpty>;
def NAME#"16ji" : MxBiArOp_RFMI<MN, NODE, MxType16, MxType16.JOp, MxType16.JPat, CMD, MxEncEAj_0, MxExtEmpty>;
def NAME#"32ji" : MxBiArOp_RFMI<MN, NODE, MxType32, MxType32.JOp, MxType32.JPat, CMD, MxEncEAj_0, MxExtEmpty>;

} // Defs = [CCR]

} // MxBiArOp_RF

// NOTE These naturally produce CCR
defm ADD : MxBiArOp_RF<"add", MxAdd, 1, 0xD, 0x6>;
defm SUB : MxBiArOp_RF<"sub", MxSub, 0, 0x9, 0x4>;


// $reg, $ccr <- $reg op $reg op $ccr
class MxBiArOp_RFRRF<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.ROp:$opd),
             MN#"."#TYPE.Prefix#"\t$opd, $dst",
             [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.VT:$opd, CCR))],
             MxArithXEncoding<MxBead4Bits<CMD>,
                              !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                              MxBead1Bit<0>, MxBeadReg<2>, MxBeadReg<0>>>;

// $reg <- $reg op $imm
class MxBiArOp_RFRIF_xEA<string MN, SDNode NODE, MxType TYPE, bits<4> CMD>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src, TYPE.IOp:$opd),
              MN#"."#TYPE.Prefix#"\t$opd, $dst",
              [(set TYPE.VT:$dst, CCR, (NODE TYPE.VT:$src, TYPE.IPat:$opd, CCR))],
              MxArithEncoding<MxBead4Bits<CMD>,
                              !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#TYPE.RLet#"EA"),
                              MxBeadReg<0>, MxEncEAi,
                              !cast<MxEncExt>("MxExtI"#TYPE.Size#"_2")>>;

multiclass MxBiArOp_RFF<string MN, SDNode NODE, bit isComm, bits<4> CMD> {

let Defs = [CCR], Uses = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = isComm in {

def NAME#"8dd"  : MxBiArOp_RFRRF<MN, NODE, MxType8d,  CMD>;
def NAME#"16dd" : MxBiArOp_RFRRF<MN, NODE, MxType16d, CMD>;
def NAME#"32dd" : MxBiArOp_RFRRF<MN, NODE, MxType32d, CMD>;

} // isCommutable = ?

} // $src = $dst

def NAME#"8di"  : MxBiArOp_RFRIF_xEA<MN, NODE, MxType8d,  CMD>;
def NAME#"16di" : MxBiArOp_RFRIF_xEA<MN, NODE, MxType16d, CMD>;
def NAME#"32ri" : MxBiArOp_RFRIF_xEA<MN, NODE, MxType32r, CMD>;

} // Defs = [CCR]

} // MxBiArOp_RFF

// NOTE These consume and produce CCR
defm ADDX : MxBiArOp_RFF<"addx", MxAddX, 1, 0xD>;
defm SUBX : MxBiArOp_RFF<"subx", MxSubX, 0, 0x9>;


//===----------------------------------------------------------------------===//
// And/Xor/Or
//===----------------------------------------------------------------------===//

// Does not allow use of An registers
multiclass MxBiArOp_DF_xEA<string MN, SDNode NODE, bits<4> CMD> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = 1 in {

def NAME#"8dd"  : MxBiArOp_RFRR_xEA<MN, NODE, MxType8d,  CMD>;
def NAME#"16dd" : MxBiArOp_RFRR_xEA<MN, NODE, MxType16d, CMD>;
def NAME#"32dd" : MxBiArOp_RFRR_xEA<MN, NODE, MxType32d, CMD>;

} // isCommutable = 1

def NAME#"8di"  : MxBiArOp_RFRI_xEA<MN, NODE,  MxType8d, CMD>;
def NAME#"16di" : MxBiArOp_RFRI_xEA<MN, NODE, MxType16d, CMD>;
def NAME#"32di" : MxBiArOp_RFRI_xEA<MN, NODE, MxType32d, CMD>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_DF_xEA

defm AND : MxBiArOp_DF_xEA<"and", MxAnd, 0xC>;
defm OR  : MxBiArOp_DF_xEA<"or",  MxOr,  0x8>;

multiclass MxBiArOp_DF_EAd<string MN, SDNode NODE, bits<4> CMD, bits<4> CMDI> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = 1 in {

def NAME#"8dd"  : MxBiArOp_RFRR_EAd<MN, NODE, MxType8d,  CMD>;
def NAME#"16dd" : MxBiArOp_RFRR_EAd<MN, NODE, MxType16d, CMD>;
def NAME#"32dd" : MxBiArOp_RFRR_EAd<MN, NODE, MxType32d, CMD>;

} // isCommutable = 1

def NAME#"8di"  : MxBiArOp_RFRI<MN, NODE,  MxType8d, CMDI>;
def NAME#"16di" : MxBiArOp_RFRI<MN, NODE, MxType16d, CMDI>;
def NAME#"32di" : MxBiArOp_RFRI<MN, NODE, MxType32d, CMDI>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_DF_EAd

defm XOR : MxBiArOp_DF_EAd<"eor", MxXor, 0xB, 0xA>;


//===----------------------------------------------------------------------===//
// Cmp
//===----------------------------------------------------------------------===//

// TODO TEST ENCODING

let Defs = [CCR] in {
class MxCmp_RR<MxType TYPE>
    : MxInst<(outs), (ins TYPE.ROp:$lhs, TYPE.ROp:$rhs),
      "cmp."#TYPE.Size#"\t$lhs, $rhs",
     [(set CCR, (MxCmp TYPE.VT:$lhs, TYPE.VT:$rhs))],
     MxArithEncoding<MxBead4Bits<0xB>,
                    !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#"dEA"),
                    MxBeadReg<1>, MxEncEAd_0, MxExtEmpty>>;

class MxCmp_RI<MxType TYPE>
    : MxInst<(outs), (ins TYPE.ROp:$reg, TYPE.IOp:$imm),
      "cmpi."#TYPE.Prefix#"\t$imm, $reg",
      [(set CCR, (MxCmp TYPE.VT:$reg, TYPE.IPat:$imm))],
      MxArithImmEncoding<MxBead4Bits<0xC>, !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                       MxEncEAd_1, MxExtEmpty,
                       !cast<MxEncExt>("MxExtI"#TYPE.Size#"_0")>>;

let mayLoad = 1 in {

class MxCmp_MI<MxType TYPE, MxOperand MEMOpd, ComplexPattern MEMPat,
               MxEncEA EA, MxEncExt EXT>
    : MxInst<(outs), (ins MEMOpd:$mem, TYPE.IOp:$imm),
      "cmpi."#TYPE.Prefix#"\t$imm, $mem",
      [(set CCR, (MxCmp (load MEMPat:$mem), TYPE.IPat:$imm))],
      MxArithImmEncoding<MxBead4Bits<0xC>, !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                       EA, EXT, !cast<MxEncExt>("MxExtI"#TYPE.Size#"_1")>>;

class MxCmp_BI<MxType TYPE>
    : MxInst<(outs), (ins MxAL32:$abs, TYPE.IOp:$imm),
      "cmpi."#TYPE.Prefix#"\t$imm, $abs",
      [(set CCR, (MxCmp (load (i32 (MxWrapper tglobaladdr:$abs))), TYPE.IPat:$imm))],
      MxArithImmEncoding<MxBead4Bits<0xC>, !cast<MxEncSize>("MxEncSize"#TYPE.Size),
                       MxEncEAb, MxExtI32_0, !cast<MxEncExt>("MxExtI"#TYPE.Size#"_1")>>;

class MxCmp_MR<MxType TYPE, MxOperand MEMOpd, ComplexPattern MEMPat,
               MxEncEA EA, MxEncExt EXT>
    : MxInst<(outs), (ins MEMOpd:$mem, TYPE.ROp:$reg),
      "cmp."#TYPE.Prefix#"\t$mem, $reg",
      [(set CCR, (MxCmp (load MEMPat:$mem), TYPE.ROp:$reg))],
      MxArithEncoding<MxBead4Bits<0xB>,
                    !cast<MxEncOpMode>("MxOpMode"#TYPE.Size#"dEA"),
                    MxBeadReg<1>, EA, EXT>>;
} // let mayLoad = 1

} // let Defs = [CCR]

multiclass MMxCmp_MR<MxType TYPE> {
def NAME#TYPE.JOp.Letter : MxCmp_MR<TYPE, TYPE.JOp, TYPE.JPat, MxEncEAj_0, MxExtEmpty>;
def NAME#TYPE.POp.Letter : MxCmp_MR<TYPE, TYPE.POp, TYPE.PPat, MxEncEAp_0, MxExtI16_0>;
def NAME#TYPE.FOp.Letter : MxCmp_MR<TYPE, TYPE.FOp, TYPE.FPat, MxEncEAf_0, MxExtBrief_0>;
}

multiclass MMxCmp_MI<MxType TYPE> {
def NAME#TYPE.JOp.Letter#"i" : MxCmp_MI<TYPE, TYPE.JOp, TYPE.JPat, MxEncEAj_0, MxExtEmpty>;
def NAME#TYPE.POp.Letter#"i" : MxCmp_MI<TYPE, TYPE.POp, TYPE.PPat, MxEncEAp_0, MxExtI16_0>;
def NAME#TYPE.FOp.Letter#"i" : MxCmp_MI<TYPE, TYPE.FOp, TYPE.FPat, MxEncEAf_0, MxExtBrief_0>;
}

foreach S = [8, 16, 32] in {
def CMP#S#dd : MxCmp_RR<!cast<MxType>("MxType"#S#"d")>;
def CMP#S#di : MxCmp_RI<!cast<MxType>("MxType"#S#"d")>;
def CMP#S#bi : MxCmp_BI<!cast<MxType>("MxType"#S#"d")>;
} // foreach

// cmp mem, Dn
defm CMP8d  : MMxCmp_MR<MxType8d>;
defm CMP16d : MMxCmp_MR<MxType16d>;
defm CMP32d : MMxCmp_MR<MxType32d>;

// cmp #imm, mem
defm CMP8  : MMxCmp_MI<MxType8d>;
defm CMP16 : MMxCmp_MI<MxType16d>;
defm CMP32 : MMxCmp_MI<MxType32d>;


//===----------------------------------------------------------------------===//
// Ext
//===----------------------------------------------------------------------===//

let Constraints = "$src = $dst" in {
  def EXT16 : MxInst<(outs MxDRD16:$dst), (ins MxDRD16:$src), "ext.w\t$src">;
  def EXT32 : MxInst<(outs MxDRD32:$dst), (ins MxDRD32:$src), "ext.l\t$src">;
} // $src = $dst

def : Pat<(sext_inreg i16:$src, i8),  (EXT16 $src)>;
def : Pat<(sext_inreg i32:$src, i16), (EXT32 $src)>;
/* def : Pat<(sext_inreg i32:$src, i8),  (EXT32 (EXT16 $src))>; */


//===----------------------------------------------------------------------===//
// Div/Mul
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxDiMuOp_DD<string MN, MxOperand DST, MxOperand OPD>
    : MxInst<(outs DST:$dst), (ins DST:$src, OPD:$opd),
           MN#"\t$opd, $dst", []>;

// $reg <- $reg op $imm
class MxDiMuOp_DI<string MN, MxOperand DST, MxOperand OPD>
    : MxInst<(outs DST:$dst), (ins DST:$src, OPD:$opd),
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
// NEG/NEGX
//===----------------------------------------------------------------------===//

// TODO encoding here

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {

class MxNeg_R<MxType TYPE>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src), "neg."#TYPE.Prefix#"\t$dst",
    [(set TYPE.VT:$dst, (ineg TYPE.VT:$src))]>;

let Uses = [CCR] in {
class MxNegX_R<MxType TYPE>
    : MxInst<(outs TYPE.ROp:$dst), (ins TYPE.ROp:$src), "negx."#TYPE.Prefix#"\t$dst",
    [(set TYPE.VT:$dst, (MxSubX 0, TYPE.VT:$src, CCR))]>;
}

} // let Constraints
} // let Defs = [CCR]

foreach S = [8, 16, 32] in {
  def NEG#S#d  : MxNeg_R<!cast<MxType>("MxType"#S#"d")>;
  def NEGX#S#d : MxNegX_R<!cast<MxType>("MxType"#S#"d")>;
}

def : Pat<(MxSub 0, i8 :$src), (NEG8d  MxDRD8 :$src)>;
def : Pat<(MxSub 0, i16:$src), (NEG16d MxDRD16:$src)>;
def : Pat<(MxSub 0, i32:$src), (NEG32d MxDRD32:$src)>;

//===----------------------------------------------------------------------===//
// no-CCR Patterns
//===----------------------------------------------------------------------===//

/// Basically the reason for this stuff is that add and addc share the same
/// operand types constraints for whatever reasons and I had to define a common
/// MxAdd and MxSub instructions that produce CCR and then pattern-map add and addc
/// to it.
/// NOTE On the other hand I see no reason why I cannot just drop explicit CCR
/// result. Anyway works for now, hopefully I will better understand how this stuff
/// is designed later
foreach node = ["add", "addc"] in {

// add reg, reg
def : Pat<(!cast<SDNode>(node) i8 :$src, i8 :$opd), (ADD8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(!cast<SDNode>(node) i16:$src, i16:$opd), (ADD16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(!cast<SDNode>(node) i32:$src, i32:$opd), (ADD32rr MxXRD32:$src, MxXRD32:$opd)>;

// add (An), reg
def : Pat<(!cast<SDNode>(node) MxType8.VT:$src, (Mxloadi8 MxType8.JPat:$opd)),
          (ADD8dj MxDRD8:$src, MxType8.JOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType16.VT:$src, (Mxloadi16 MxType16.JPat:$opd)),
          (ADD16dj MxDRD16:$src, MxType16.JOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType32.VT:$src, (Mxloadi32 MxType32.JPat:$opd)),
          (ADD32rj MxXRD32:$src, MxType32.JOp:$opd)>;

// add (i,An), reg
def : Pat<(!cast<SDNode>(node) MxType8.VT:$src, (Mxloadi8 MxType8.PPat:$opd)),
          (ADD8dp MxDRD8:$src, MxType8.POp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType16.VT:$src, (Mxloadi16 MxType16.PPat:$opd)),
          (ADD16dp MxDRD16:$src, MxType16.POp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType32.VT:$src, (Mxloadi32 MxType32.PPat:$opd)),
          (ADD32rp MxXRD32:$src, MxType32.POp:$opd)>;

// add (i,An,Xn), reg
def : Pat<(!cast<SDNode>(node) MxType8.VT:$src, (Mxloadi8 MxType8.FPat:$opd)),
          (ADD8df MxDRD8:$src, MxType8.FOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType16.VT:$src, (Mxloadi16 MxType16.FPat:$opd)),
          (ADD16df MxDRD16:$src, MxType16.FOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType32.VT:$src, (Mxloadi32 MxType32.FPat:$opd)),
          (ADD32rf MxXRD32:$src, MxType32.FOp:$opd)>;

// add reg, imm
def : Pat<(!cast<SDNode>(node) i8: $src, MximmSExt8:$opd),  (ADD8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(!cast<SDNode>(node) i16:$src, MximmSExt16:$opd), (ADD16di MxDRD16:$src, imm:$opd)>;
def : Pat<(!cast<SDNode>(node) i32:$src, MximmSExt32:$opd), (ADD32ri MxXRD32:$src, imm:$opd)>;

// add imm, (An)
def : Pat<(store (!cast<SDNode>(node) (load MxType8.JPat:$dst), MxType8.IPat:$opd), MxType8.JPat:$dst),
          (ADD8ji MxType8.JOp:$dst, imm:$opd)>;
def : Pat<(store (!cast<SDNode>(node) (load MxType16.JPat:$dst), MxType16.IPat:$opd), MxType16.JPat:$dst),
          (ADD16ji MxType16.JOp:$dst, imm:$opd)>;
def : Pat<(store (!cast<SDNode>(node) (load MxType32.JPat:$dst), MxType32.IPat:$opd), MxType32.JPat:$dst),
          (ADD32ji MxType32.JOp:$dst, imm:$opd)>;

} // foreach add, addc

def : Pat<(adde i8 :$src, i8 :$opd), (ADDX8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(adde i16:$src, i16:$opd), (ADDX16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(adde i32:$src, i32:$opd), (ADDX32dd MxDRD32:$src, MxDRD32:$opd)>;


foreach node = ["sub", "subc"] in {

// sub reg, reg
def : Pat<(!cast<SDNode>(node) i8 :$src, i8 :$opd), (SUB8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(!cast<SDNode>(node) i16:$src, i16:$opd), (SUB16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(!cast<SDNode>(node) i32:$src, i32:$opd), (SUB32rr MxXRD32:$src, MxXRD32:$opd)>;


// sub (An), reg
def : Pat<(!cast<SDNode>(node) MxType8.VT:$src, (Mxloadi8 MxType8.JPat:$opd)),
          (SUB8dj MxDRD8:$src, MxType8.JOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType16.VT:$src, (Mxloadi16 MxType16.JPat:$opd)),
          (SUB16dj MxDRD16:$src, MxType16.JOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType32.VT:$src, (Mxloadi32 MxType32.JPat:$opd)),
          (SUB32rj MxXRD32:$src, MxType32.JOp:$opd)>;

// sub (i,An), reg
def : Pat<(!cast<SDNode>(node) MxType8.VT:$src, (Mxloadi8 MxType8.PPat:$opd)),
          (SUB8dp MxDRD8:$src, MxType8.POp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType16.VT:$src, (Mxloadi16 MxType16.PPat:$opd)),
          (SUB16dp MxDRD16:$src, MxType16.POp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType32.VT:$src, (Mxloadi32 MxType32.PPat:$opd)),
          (SUB32rp MxXRD32:$src, MxType32.POp:$opd)>;

// sub (i,An,Xn), reg
def : Pat<(!cast<SDNode>(node) MxType8.VT:$src, (Mxloadi8 MxType8.FPat:$opd)),
          (SUB8df MxDRD8:$src, MxType8.FOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType16.VT:$src, (Mxloadi16 MxType16.FPat:$opd)),
          (SUB16df MxDRD16:$src, MxType16.FOp:$opd)>;
def : Pat<(!cast<SDNode>(node) MxType32.VT:$src, (Mxloadi32 MxType32.FPat:$opd)),
          (SUB32rf MxXRD32:$src, MxType32.FOp:$opd)>;

// sub reg, imm
def : Pat<(!cast<SDNode>(node) i8 :$src, MximmSExt8 :$opd), (SUB8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(!cast<SDNode>(node) i16:$src, MximmSExt16:$opd), (SUB16di MxDRD16:$src, imm:$opd)>;
def : Pat<(!cast<SDNode>(node) i32:$src, MximmSExt32:$opd), (SUB32ri MxXRD32:$src, imm:$opd)>;

// sub imm, (An)
def : Pat<(store (!cast<SDNode>(node) (load MxType8.JPat:$dst), MxType8.IPat:$opd), MxType8.JPat:$dst),
          (SUB8ji MxType8.JOp:$dst, imm:$opd)>;
def : Pat<(store (!cast<SDNode>(node) (load MxType16.JPat:$dst), MxType16.IPat:$opd), MxType16.JPat:$dst),
          (SUB16ji MxType16.JOp:$dst, imm:$opd)>;
def : Pat<(store (!cast<SDNode>(node) (load MxType32.JPat:$dst), MxType32.IPat:$opd), MxType32.JPat:$dst),
          (SUB32ji MxType32.JOp:$dst, imm:$opd)>;

} // foreach sub, subx

def : Pat<(sube i8 :$src, i8 :$opd), (SUBX8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(sube i16:$src, i16:$opd), (SUBX16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(sube i32:$src, i32:$opd), (SUBX32dd MxDRD32:$src, MxDRD32:$opd)>;


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
