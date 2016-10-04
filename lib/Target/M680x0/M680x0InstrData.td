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

class MxMove<string size, dag outs, dag ins, list<dag> pattern = [(null_frag)]>
    : MxInst<0x0, outs, ins, "move."#size#"\t$src, $dst", pattern>;


class MxMove_RR<MxType DST, MxType SRC>
    : MxMove<DST.Prefix, (outs DST.ROp:$dst), (ins SRC.ROp:$src)>;

class MxMove_MR<MxOperand MEMOpd, ComplexPattern MEMPat, MxType REG>
    : MxMove<REG.Prefix, (outs), (ins MEMOpd:$dst, REG.ROp:$src),
             [(store REG.VT:$src, MEMPat:$dst)]>;

class MxMove_RI<MxType DST>
    : MxMove<DST.Prefix, (outs DST.ROp:$dst), (ins DST.IOp:$src),
              [(set DST.VT:$dst, DST.IPat:$src)]>;


class MxMove_RM<MxType REG, MxOperand MEMOpd, ComplexPattern MEMPat>
    : MxMove<REG.Prefix, (outs REG.ROp:$dst), (ins MEMOpd:$src),
             [(set REG.VT:$dst, (REG.Load MEMPat:$src))]>;

multiclass MMxMove_RM<MxType REG> {

// REG <- (An)
def NAME#REG.JOp.Letter#REG.Postfix : MxMove_RM<REG, REG.JOp, REG.JPat>;

// REG <- (An)+
def NAME#REG.OOp.Letter#REG.Postfix : MxMove_RM<REG, REG.OOp, REG.OPat>;

// REG <- -(An)
def NAME#REG.EOp.Letter#REG.Postfix : MxMove_RM<REG, REG.EOp, REG.EPat>;

// REG <- (i,An)
def NAME#REG.POp.Letter#REG.Postfix : MxMove_RM<REG, REG.POp, REG.PPat>;

// REG <- (i,An,Xn)
def NAME#REG.FOp.Letter#REG.Postfix : MxMove_RM<REG, REG.FOp, REG.FPat>;

}


class MxMove_MM<string SIZE, PatFrag LOAD,
                MxOperand DSTOpd, ComplexPattern DSTPat,
                MxOperand SRCOpd, ComplexPattern SRCPat>
    : MxMove<SIZE, (outs), (ins DSTOpd:$dst, SRCOpd:$src),
             [(store (LOAD SRCPat:$src), DSTPat:$dst)]>;

multiclass MMxMove_MM<MxType TYPE, MxOperand DSTOpd, ComplexPattern DSTPat> {

// MEM <- (An)
def NAME#TYPE.JOp.Letter#TYPE.Postfix
  : MxMove_MM<TYPE.Prefix, TYPE.Load, DSTOpd, DSTPat, TYPE.JOp, TYPE.JPat>;

// MEM <- (An)+
def NAME#TYPE.OOp.Letter#TYPE.Postfix
  : MxMove_MM<TYPE.Prefix, TYPE.Load, DSTOpd, DSTPat, TYPE.OOp, TYPE.OPat>;

// MEM <- -(An)
def NAME#TYPE.EOp.Letter#TYPE.Postfix
  : MxMove_MM<TYPE.Prefix, TYPE.Load, DSTOpd, DSTPat, TYPE.EOp, TYPE.EPat>;

// MEM <- (i,An)
def NAME#TYPE.POp.Letter#TYPE.Postfix
  : MxMove_MM<TYPE.Prefix, TYPE.Load, DSTOpd, DSTPat, TYPE.POp, TYPE.PPat>;

// MEM <- (i,An,Xn)
def NAME#TYPE.FOp.Letter#TYPE.Postfix
  : MxMove_MM<TYPE.Prefix, TYPE.Load, DSTOpd, DSTPat, TYPE.FOp, TYPE.FPat>;

}

def  MOV8dd : MxMove_RR<MxType8d, MxType8d>;

def  MOV8jd : MxMove_MR<MxType8.JOp, MxType8.JPat, MxType8d>;
def  MOV8fd : MxMove_MR<MxType8.FOp, MxType8.FPat, MxType8d>;
def  MOV8pd : MxMove_MR<MxType8.POp, MxType8.PPat, MxType8d>;

foreach S = [16, 32] in {
  foreach D = [ "d", "a", "r" ] in {
    foreach O = [ "d", "a", "r" ] in {
      def MOV#S#D#O : MxMove_RR<
        !cast<MxType>("MxType"#S#D),
        !cast<MxType>("MxType"#S#O)>;
    }

    def MOV#S#"j"#D : MxMove_MR<
      !cast<MxType>("MxType"#S).JOp,
      !cast<MxType>("MxType"#S).JPat,
      !cast<MxType>("MxType"#S#D)>;

    def MOV#S#"f"#D : MxMove_MR<
      !cast<MxType>("MxType"#S).FOp,
      !cast<MxType>("MxType"#S).FPat,
      !cast<MxType>("MxType"#S#D)>;

    def MOV#S#"p"#D : MxMove_MR<
      !cast<MxType>("MxType"#S).POp,
      !cast<MxType>("MxType"#S).PPat,
      !cast<MxType>("MxType"#S#D)>;

    def MOV#S#D#"i" : MxMove_RI<
      !cast<MxType>("MxType"#S#D)>;
  }
}

// FIXME Tablegen needs to parse complex defm names inside the loop FFS

// R <- M
defm MOV8d  : MMxMove_RM<MxType8d>;

defm MOV16d : MMxMove_RM<MxType16d>;
defm MOV16a : MMxMove_RM<MxType16a>;
defm MOV16r : MMxMove_RM<MxType16r>;

defm MOV32d : MMxMove_RM<MxType32d>;
defm MOV32a : MMxMove_RM<MxType32a>;
defm MOV32r : MMxMove_RM<MxType32r>;

defm MOV16d : MMxMove_RM<MxType16d_TC>;
defm MOV16a : MMxMove_RM<MxType16a_TC>;
defm MOV16r : MMxMove_RM<MxType16r_TC>;

defm MOV32d : MMxMove_RM<MxType32d_TC>;
defm MOV32a : MMxMove_RM<MxType32a_TC>;
defm MOV32r : MMxMove_RM<MxType32r_TC>;

// M <- M
defm MOV8j  : MMxMove_MM<MxType8,  MxType8.JOp,  MxType8.JPat>;
defm MOV16j : MMxMove_MM<MxType16, MxType16.JOp, MxType16.JPat>;
defm MOV32j : MMxMove_MM<MxType32, MxType32.JOp, MxType32.JPat>;

defm MOV8f  : MMxMove_MM<MxType8,  MxType8.FOp,  MxType8.FPat>;
defm MOV16f : MMxMove_MM<MxType16, MxType16.FOp, MxType16.FPat>;
defm MOV32f : MMxMove_MM<MxType32, MxType32.FOp, MxType32.FPat>;


//===----------------------------------------------------------------------===//
// PUSH
//===----------------------------------------------------------------------===//

// FIXME Make it nicier, mb as Pseudos?
let Defs = [SP], hasSideEffects = 0 in {

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


//===----------------------------------------------------------------------===//
// LEA
//===----------------------------------------------------------------------===//

class LEA32<MxOperand SRCOpd, ComplexPattern SRCPat>
    : MxInst<0x0, (outs MxARD32:$dst), (ins SRCOpd:$src),
             "lea\t$src, $dst", [(set i32:$dst, SRCPat:$src)]>;

def LEA32p : LEA32<MxARID32, MxCP_ARID>;
def LEA32f : LEA32<MxARII32, MxCP_ARII>;


//===----------------------------------------------------------------------===//
// Pseudos
//===----------------------------------------------------------------------===//

class MxPseudoMove_RR<MxType DST, MxType SRC, list<dag> PAT = []>
    : MxPseudo<(outs DST.ROp:$dst), (ins SRC.ROp:$src), "", PAT>;

class MxPseudoMove_RM<MxType DST, MxOperand SRCOpd, list<dag> PAT = []>
    : MxPseudo<(outs DST.ROp:$dst), (ins SRCOpd:$src), "", PAT>;

/// This group of Pseudos is analogues to the real x86 extending moves, but
/// since M680x0 does not have those we need to emulate. These instructions
/// will be expanded right after RA completed because we need to know precisely
/// what registers are allocated for the operands and if they overlap we just
/// extend the value if the registers are completely different we need to move
/// first.
/// TODO What is the CCR behaviour here?
foreach EXT = ["S", "Z"] in {
  let hasSideEffects = 0 in {

    def MOV#EXT#Xd16d8  : MxPseudoMove_RR<MxType16d,  MxType8d>;
    def MOV#EXT#Xd32d8  : MxPseudoMove_RR<MxType32d,  MxType8d>;
    def MOV#EXT#Xd32d16 : MxPseudoMove_RR<MxType32r, MxType16r>;

    let mayLoad = 1 in {

      def MOV#EXT#Xd16j8   : MxPseudoMove_RM<MxType16r,  MxType8.JOp>;
      def MOV#EXT#Xd32j8   : MxPseudoMove_RM<MxType32r,  MxType8.JOp>;
      def MOV#EXT#Xd32j16  : MxPseudoMove_RM<MxType32r, MxType16.JOp>;

      def MOV#EXT#Xd16p8   : MxPseudoMove_RM<MxType16r,  MxType8.POp>;
      def MOV#EXT#Xd32p8   : MxPseudoMove_RM<MxType32r,  MxType8.POp>;
      def MOV#EXT#Xd32p16  : MxPseudoMove_RM<MxType32r, MxType16.POp>;

      def MOV#EXT#Xd16f8   : MxPseudoMove_RM<MxType16r,  MxType8.FOp>;
      def MOV#EXT#Xd32f8   : MxPseudoMove_RM<MxType32r,  MxType8.FOp>;
      def MOV#EXT#Xd32f16  : MxPseudoMove_RM<MxType32r, MxType16.FOp>;

    }
  }
}


//===----------------------------------------------------------------------===//
// Extensions
//===----------------------------------------------------------------------===//

// i16 <- i8
def: Pat<(i16 (sext i8:$src)),
          (EXTRACT_SUBREG (MOVSXd32d8 MxDRD8:$src), MxSubRegIndex16Lo)>;

def: Pat<(i16 (zext i8:$src)),
          (EXTRACT_SUBREG (MOVZXd32d8 MxDRD8:$src), MxSubRegIndex16Lo)>;

def: Pat<(MxSExtLoadi16i8 MxCP_ARI:$src),
          (EXTRACT_SUBREG (MOVSXd32j8 MxARI8:$src), MxSubRegIndex16Lo)>;

def: Pat<(MxZExtLoadi16i8 MxCP_ARI:$src),
          (EXTRACT_SUBREG (MOVZXd32j8 MxARI8:$src), MxSubRegIndex16Lo)>;

// i32 <- i16
def: Pat<(i32 (sext i16:$src)), (MOVSXd32d16 MxDRD16:$src)>;
def: Pat<(i32 (zext i16:$src)), (MOVZXd32d16 MxDRD16:$src)>;
def: Pat<(MxSExtLoadi32i16 MxCP_ARI:$src), (MOVSXd32j16 MxARI16:$src)>;
def: Pat<(MxZExtLoadi32i16 MxCP_ARI:$src), (MOVZXd32j16 MxARI16:$src)>;

// trunc patterns
def : Pat<(i16 (trunc i32:$src)),
          (EXTRACT_SUBREG MxXRD32:$src, MxSubRegIndex16Lo)>;
def : Pat<(i8  (trunc i32:$src)),
          (EXTRACT_SUBREG MxXRD32:$src, MxSubRegIndex8Lo)>;
def : Pat<(i8  (trunc i16:$src)),
          (EXTRACT_SUBREG MxXRD16:$src, MxSubRegIndex8Lo)>;
