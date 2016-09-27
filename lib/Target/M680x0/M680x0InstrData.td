//== M680x0InstrData.td - M680x0 Data Movement Instructions  -- tablegen --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Motorola 680x0 data movement instructions which are
// the basic means of transferring and storing addresses and data. Here is the
// current status of the file:
//
//  Machine:
//
//     EXG   [ ]     FMOVE [ ]     FSMOVE [ ]     FDMOVE [ ]     FMOVEM [ ]
//     LEA   [~]     PEA   [ ]     MOVE   [!]     MOVE16 [ ]     MOVEA  [ ]
//     MOVEM [ ]     MOVEP [ ]     MOVEQ  [ ]     LINK   [ ]     UNLK   [ ]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - in progress but usable
//   [x] - done
//
//===----------------------------------------------------------------------===//


// TODO make move set imlicit CCR like M680x0 does

multiclass MxMOV8Inst<MxOperand OPD, dag PAT> {

// X - not affected
// N,Z - set by operation
// V,C - cleared
let Defs = [CCR] in {
let mayLoad = 1 in  {

// Dn <- OPD
def "MOV8d"#OPD.Letter
  : MxInst<0x0, (outs MxDRD8:$dst), (ins OPD:$src), "move.b\t$src, $dst",
           [(set i8:$dst, PAT)]>;

} // mayLoad = 1
} // Defs = [CCR]

// (An) <- OPD
def "MOV8j"#OPD.Letter
  : MxInst<0x0, (outs), (ins MxARI8:$dst, OPD:$src), "move.b\t$src, $dst",
           [(store PAT, iPTR:$dst)]>;

// (i,An) <- OPD
def "MOV8p"#OPD.Letter
  : MxInst<0x0, (outs), (ins MxARID8:$dst, OPD:$src), "move.b\t$src, $dst",
           [(store PAT, MxCP_ARID:$dst)]>;
}

multiclass MxMOVInst<MxOperand OPD, dag PAT> {

// X - not affected
// N,Z - set by operation
// V,C - cleared
let Defs = [CCR] in {
let mayLoad = 1 in  {

// Set CS registers
def "MOV"#OPD.Size.Num#"d"#OPD.Letter
  : MxInst<0x0, (outs !cast<DAGOperand>("MxDRD"#OPD.Size.Num):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"a"#OPD.Letter
  : MxInst<0x0, (outs !cast<DAGOperand>("MxARD"#OPD.Size.Num):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"r"#OPD.Letter
  : MxInst<0x0, (outs !cast<DAGOperand>("MxXRD"#OPD.Size.Num):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;


// Set TC registers

def "MOV"#OPD.Size.Num#"d"#OPD.Letter#"_TC"
  : MxInst<0x0, (outs !cast<DAGOperand>("MxDRD"#OPD.Size.Num#"_TC"):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"a"#OPD.Letter#"_TC"
  : MxInst<0x0, (outs !cast<DAGOperand>("MxARD"#OPD.Size.Num#"_TC"):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

def "MOV"#OPD.Size.Num#"r"#OPD.Letter#"_TC"
  : MxInst<0x0, (outs !cast<DAGOperand>("MxXRD"#OPD.Size.Num#"_TC"):$dst), (ins OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(set !cast<DAGOperand>("i"#OPD.Size.Num):$dst, PAT)]>;

} // mayLoad = 1
} // Defs = [CCR]


// Store to MEM

// (An) <- OPD
def "MOV"#OPD.Size.Num#"j"#OPD.Letter
  : MxInst<0x0, (outs), (ins !cast<DAGOperand>("MxARI"#OPD.Size.Num):$dst, OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(store PAT, iPTR:$dst)]>;

// (i,An) <- OPD
def "MOV"#OPD.Size.Num#"p"#OPD.Letter
  : MxInst<0x0, (outs), (ins !cast<DAGOperand>("MxARID"#OPD.Size.Num):$dst, OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(store PAT, MxCP_ARID:$dst)]>;

// (i,An,Xn) <- OPD
def "MOV"#OPD.Size.Num#"f"#OPD.Letter
  : MxInst<0x0, (outs), (ins !cast<DAGOperand>("MxARII"#OPD.Size.Num):$dst, OPD:$src),
           "move."#OPD.Size.Id#"\t$src, $dst",
           [(store PAT, MxCP_ARII:$dst)]>;
}

defm MOV8_d  : MxMOV8Inst<MxDRD8, (i8  imm:$src)>;
defm MOV16_d : MxMOVInst<MxDRD16, (i16 imm:$src)>;
defm MOV32_d : MxMOVInst<MxDRD32, (i32 imm:$src)>;

defm MOV16_a : MxMOVInst<MxARD16, (i16 imm:$src)>;
defm MOV32_a : MxMOVInst<MxARD32, (i32 imm:$src)>;

defm MOV16_r : MxMOVInst<MxXRD16, (i16 imm:$src)>;
defm MOV32_r : MxMOVInst<MxXRD32, (i32 imm:$src)>;

defm MOV8_i  : MxMOV8Inst<Mxi8imm, (i8 MximmSExt8:$src)>;
defm MOV16_i : MxMOVInst<Mxi16imm, (i16 MximmSExt16:$src)>;
defm MOV32_i : MxMOVInst<Mxi32imm, (i32 MximmSExt32:$src)>;

defm MOV8_j  : MxMOV8Inst<MxARI8, (Mxloadi8  iPTR:$src)>;
defm MOV16_j : MxMOVInst<MxARI16, (Mxloadi16 iPTR:$src)>;
defm MOV32_j : MxMOVInst<MxARI32, (Mxloadi32 iPTR:$src)>;

defm MOV8_p  : MxMOV8Inst<MxARID8, (Mxloadi8  MxCP_ARID:$src)>;
defm MOV16_p : MxMOVInst<MxARID16, (Mxloadi16 MxCP_ARID:$src)>;
defm MOV32_p : MxMOVInst<MxARID32, (Mxloadi32 MxCP_ARID:$src)>;

defm MOV8_f  : MxMOV8Inst<MxARII8, (Mxloadi8  MxCP_ARII:$src)>;
defm MOV16_f : MxMOVInst<MxARII16, (Mxloadi16 MxCP_ARII:$src)>;
defm MOV32_f : MxMOVInst<MxARII32, (Mxloadi32 MxCP_ARII:$src)>;


// FIXME Could these be specialized from normal MOVrPIm and MOVrpDm?
let Defs = [SP], hasSideEffects = 0 in {

// FIXME
// FIXME Solve bulshit with SP
// FIXME
// SP <- SP - <size>; (SP) <- Dn
let mayStore = 1 in {
def PUSH8r  : MxInst<0x0, (outs), (ins DR8:$reg),
                         "move.b\t$reg, -(SP)",[]>;
def PUSH16r : MxInst<0x0, (outs), (ins DR16:$reg),
                         "move.w\t$reg, -(SP)",[]>;
def PUSH32r : MxInst<0x0, (outs), (ins AR32:$reg),
                         "move.l\t$reg, -(SP)",[]>;
} // let mayStore = 1

// Dn <- (SP); SP <- SP + <size>
let mayLoad = 1 in {
def POP8r  : MxInst<0x0, (outs DR8:$reg),  (ins),
                        "move.b\t(SP)+, $reg", []>;
def POP16r : MxInst<0x0, (outs DR16:$reg), (ins),
                        "move.w\t(SP)+, $reg", []>;
def POP32r : MxInst<0x0, (outs AR32:$reg), (ins),
                        "move.l\t(SP)+, $reg", []>;
} // let mayLoad = 1

} // let Defs = [SP], hasSideEffects = 0

// LEA - Load Effective Address
def LEA32r_ARID : MxInst<0x0, (outs AR32:$dst), (ins MxARID32:$src),
                        "lea\t$src, $dst",
                        [(set i32:$dst, MxCP_ARID:$src)]>;

def LEA32r_ARII : MxInst<0x0, (outs AR32:$dst), (ins MxARII32:$src),
                        "lea\t$src, $dst",
                        [(set i32:$dst, MxCP_ARII:$src)]>;


