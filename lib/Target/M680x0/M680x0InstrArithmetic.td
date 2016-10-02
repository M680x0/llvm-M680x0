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
// Add/Sub
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxBiArOp_RFRR<string mnemonic, MxOperand DST, MxOperand OPD, SDNode node>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           mnemonic#"."#DST.Size.Id#"\t$opd, $dst",
           [(set DST.VT:$dst, CCR, (node DST.VT:$src, OPD.VT:$opd))]>;

// $reg <- $reg op $imm
class MxBiArOp_RFRI<string mnemonic, MxOperand DST, MxOperand OPD,
                    SDPatternOperator PAT, SDNode node>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           mnemonic#"."#DST.Size.Id#"\t$opd, $dst",
           [(set DST.VT:$dst, CCR, (node DST.VT:$src, PAT:$opd))]>;


// Allows use of An registers
multiclass MxBiArOp_RF<string mnemonic, SDNode mxnode, SDNode node> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = 1 in {

def NAME#"8dd"  : MxBiArOp_RFRR<mnemonic, MxDRD8,  MxDRD8,  mxnode>;
def NAME#"16rr" : MxBiArOp_RFRR<mnemonic, MxXRD16, MxXRD16, mxnode>;
def NAME#"32rr" : MxBiArOp_RFRR<mnemonic, MxXRD32, MxXRD32, mxnode>;

} // isCommutable = 1

def NAME#"8di"  : MxBiArOp_RFRI<mnemonic, MxDRD8,  Mxi8imm,  MximmSExt8,  mxnode>;
def NAME#"16ri" : MxBiArOp_RFRI<mnemonic, MxXRD16, Mxi16imm, MximmSExt16, mxnode>;
def NAME#"32ri" : MxBiArOp_RFRI<mnemonic, MxXRD32, Mxi32imm, MximmSExt32, mxnode>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_RF

defm ADD : MxBiArOp_RF<"add", MxAdd, add>;
defm SUB : MxBiArOp_RF<"sub", MxSub, sub>;

//===----------------------------------------------------------------------===//
// And/Xor/Or
//===----------------------------------------------------------------------===//

// Does not allow use of An registers
multiclass MxBiArOp_DF<string mnemonic, SDNode mxnode, SDNode node> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {
let isCommutable = 1 in {

def NAME#"8dd"  : MxBiArOp_RFRR<mnemonic, MxDRD8,  MxDRD8,  mxnode>;
def NAME#"16dd" : MxBiArOp_RFRR<mnemonic, MxDRD16, MxDRD16, mxnode>;
def NAME#"32dd" : MxBiArOp_RFRR<mnemonic, MxDRD32, MxDRD32, mxnode>;

} // isCommutable = 1

def NAME#"8di"  : MxBiArOp_RFRI<mnemonic, MxDRD8,  Mxi8imm,  MximmSExt8,  mxnode>;
def NAME#"16di" : MxBiArOp_RFRI<mnemonic, MxDRD16, Mxi16imm, MximmSExt16, mxnode>;
def NAME#"32di" : MxBiArOp_RFRI<mnemonic, MxDRD32, Mxi32imm, MximmSExt32, mxnode>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_RF

defm AND : MxBiArOp_DF<"and", MxAnd, and>;
defm XOR : MxBiArOp_DF<"eor", MxXor, xor>;
defm OR  : MxBiArOp_DF<"or",  MxOr,  or>;

//===----------------------------------------------------------------------===//
// Ext
//===----------------------------------------------------------------------===//

let Constraints = "$src = $dst" in {

def EXT16 : MxInst<0x0, (outs MxDRD16:$dst), (ins MxDRD8 :$src), "ext.w\t$src">;
def EXT32 : MxInst<0x0, (outs MxDRD32:$dst), (ins MxDRD16:$src), "ext.l\t$src">;

} // $src = $dst

def : Pat<(i16 (sext i8 :$src)), (EXT16 $src)>;
def : Pat<(i32 (sext i16:$src)), (EXT32 $src)>;

//===----------------------------------------------------------------------===//
// Div/Mul
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxDiMuOp_DD<string mnemonic, MxOperand DST, MxOperand OPD>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           mnemonic#"\t$opd, $dst", []>;

// $reg <- $reg op $imm
class MxDiMuOp_DI<string mnemonic, MxOperand DST, MxOperand OPD>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, OPD:$opd),
           mnemonic#"\t$opd, $dst", []>;

multiclass MxDiMuOp<string mnemonic> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {

def "S"#NAME#"d32d16" : MxDiMuOp_DD<mnemonic#"s", MxDRD32, MxDRD16>;
def "U"#NAME#"d32d16" : MxDiMuOp_DD<mnemonic#"u", MxDRD32, MxDRD16>;

def "S"#NAME#"d32i16" : MxDiMuOp_DI<mnemonic#"s", MxDRD32, Mxi16imm>;
def "U"#NAME#"d32i16" : MxDiMuOp_DI<mnemonic#"u", MxDRD32, Mxi16imm>;

} // $src = $dst
} // Defs = [CCR]

}

defm DIV : MxDiMuOp<"div">;

def : Pat<(sdiv i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (SDIVd32d16 (EXT32 $dst), $opd),
            MxSubRegIndex16Lo)>;

def : Pat<(udiv i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (UDIVd32d16 (EXT32 $dst), $opd),
            MxSubRegIndex16Lo)>;

def : Pat<(srem i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (ASR32di (ASR32di (SDIVd32d16 (EXT32 $dst), $opd), 8), 8),
            MxSubRegIndex16Lo)>;

def : Pat<(urem i16:$dst, i16:$opd),
          (EXTRACT_SUBREG
            (LSR32di (LSR32di (UDIVd32d16 (EXT32 $dst), $opd), 8), 8),
            MxSubRegIndex16Lo)>;


defm MUL : MxDiMuOp<"mul">;


//===----------------------------------------------------------------------===//
// no-CCR Patterns
//===----------------------------------------------------------------------===//

// TODO figure out why the fuck i cannot use Mxi8/16/32imm as operand,
// but only imm.

// add reg, reg
def : Pat<(add i8 :$src, i8 :$opd), (ADD8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(add i16:$src, i16:$opd), (ADD16rr MxXRD16:$src, MxXRD16:$opd)>;
def : Pat<(add i32:$src, i32:$opd), (ADD32rr MxXRD32:$src, MxXRD32:$opd)>;

/* // add reg, mem */
/* def : Pat<(add GR8:$src, (loadi8 addr:$opd)), */
/*           (ADD8rm GR8:$src, addr:$opd)>; */
/* def : Pat<(add GR16:$src, (loadi16 addr:$opd)), */
/*           (ADD16rm GR16:$src, addr:$opd)>; */
/* def : Pat<(add GR32:$src, (loadi32 addr:$opd)), */
/*           (ADD32rm GR32:$src, addr:$opd)>; */

// add reg, imm
def : Pat<(add i8: $src, MximmSExt8:$opd) , (ADD8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(add i16:$src, MximmSExt16:$opd), (ADD16ri MxXRD16:$src, imm:$opd)>;
def : Pat<(add i32:$src, MximmSExt32:$opd), (ADD32ri MxXRD32:$src, imm:$opd)>;

// TODO checkout if i need a similar patters and for sub too
/* def : Pat<(add GR16:$src, i16immSExt8:$opd), */
/*           (ADD16ri8 GR16:$src, i16immSExt8:$opd)>; */
/* def : Pat<(add GR32:$src, i32immSExt8:$opd), */
/*           (ADD32ri8 GR32:$src, i32immSExt8:$opd)>; */

// sub reg, reg
def : Pat<(sub i8 :$src, i8 :$opd), (SUB8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(sub i16:$src, i16:$opd), (SUB16rr MxXRD16:$src, MxXRD16:$opd)>;
def : Pat<(sub i32:$src, i32:$opd), (SUB32rr MxXRD32:$src, MxXRD32:$opd)>;

/* // sub reg, mem */
/* def : Pat<(sub GR8:$src, (loadi8 addr:$opd)), */
/*           (SUB8rm GR8:$src, addr:$opd)>; */
/* def : Pat<(sub GR16:$src, (loadi16 addr:$opd)), */
/*           (SUB16rm GR16:$src, addr:$opd)>; */
/* def : Pat<(sub GR32:$src, (loadi32 addr:$opd)), */
/*           (SUB32rm GR32:$src, addr:$opd)>; */

// sub reg, imm
def : Pat<(sub i8 :$src, MximmSExt8 :$opd), (SUB8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(sub i16:$src, MximmSExt16:$opd), (SUB16ri MxXRD16:$src, imm:$opd)>;
def : Pat<(sub i32:$src, MximmSExt32:$opd), (SUB32ri MxXRD32:$src, imm:$opd)>;

// and reg, reg
def : Pat<(and i8 :$src, i8 :$opd), (AND8dd  MxDRD8 :$src, MxDRD8 :$opd)>;
def : Pat<(and i16:$src, i16:$opd), (AND16dd MxDRD16:$src, MxDRD16:$opd)>;
def : Pat<(and i32:$src, i32:$opd), (AND32dd MxDRD32:$src, MxDRD32:$opd)>;

// and reg, mem
/* def : Pat<(and GR8:$src, (loadi8 addr:$opd)), */
/*           (AND8rm GR8:$src, addr:$opd)>; */
/* def : Pat<(and GR16:$src, (loadi16 addr:$opd)), */
/*           (AND16rm GR16:$src, addr:$opd)>; */
/* def : Pat<(and GR32:$src, (loadi32 addr:$opd)), */
/*           (AND32rm GR32:$src, addr:$opd)>; */
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
/* def : Pat<(xor GR8:$src, (loadi8 addr:$opd)), */
/*           (XOR8rm GR8:$src, addr:$opd)>; */
/* def : Pat<(xor GR16:$src, (loadi16 addr:$opd)), */
/*           (XOR16rm GR16:$src, addr:$opd)>; */
/* def : Pat<(xor GR32:$src, (loadi32 addr:$opd)), */
/*           (XOR32rm GR32:$src, addr:$opd)>; */
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
/* def : Pat<(or GR8:$src, (loadi8 addr:$opd)), */
/*           (OR8rm GR8:$src, addr:$opd)>; */
/* def : Pat<(or GR16:$src, (loadi16 addr:$opd)), */
/*           (OR16rm GR16:$src, addr:$opd)>; */
/* def : Pat<(or GR32:$src, (loadi32 addr:$opd)), */
/*           (OR32rm GR32:$src, addr:$opd)>; */
/* def : Pat<(or GR64:$src, (loadi64 addr:$opd)), */
/*           (OR64rm GR64:$src, addr:$opd)>; */

// or reg, imm
def : Pat<(or i8: $src, MximmSExt8 :$opd), (OR8di  MxDRD8 :$src, imm:$opd)>;
def : Pat<(or i16:$src, MximmSExt16:$opd), (OR16di MxDRD16:$src, imm:$opd)>;
def : Pat<(or i32:$src, MximmSExt32:$opd), (OR32di MxDRD32:$src, imm:$opd)>;
