//===------ M680x0InstrShiftRotate.td - Logical Instrs ------- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the logical instructions in the M680x0 architecture.
// Here is the current status of the file:
//
//  Machine:
//
//    SHL     [~]   ASR     [~]   LSR      [~]   SWAP     [ ]
//    ROL     [~]   ROR     [~]   ROXL     [ ]   ROXR     [ ]
//
//  Map:
//
//   [ ] - was not touched at all
//   [!] - requires extarnal stuff implemented
//   [~] - in progress but usable
//   [x] - done
//
//===----------------------------------------------------------------------===//

// $reg <- $reg op $reg
class MxSROp_DD<string mnemonic, MxOperand DST, SDNode node>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, DST:$opd),
           mnemonic#"."#DST.Size.Id#"\t$opd, $dst",
           [(set DST.VT:$dst, (node DST.VT:$src, DST.VT:$opd))]>;

// $reg <- $reg op $imm
class MxSROp_DI<string mnemonic, MxOperand DST, SDNode node>
    : MxInst<0x0, (outs DST:$dst), (ins DST:$src, Mxi32imm:$opd),
           mnemonic#"."#DST.Size.Id#"\t$opd, $dst",
           [(set DST.VT:$dst, (node DST.VT:$src, Mximm1to8:$opd))]>;

multiclass MxSROp<string mnemonic, SDNode node> {

let Defs = [CCR] in {
let Constraints = "$src = $dst" in {

def NAME#"8dd"  : MxSROp_DD<mnemonic, MxDRD8,  node>;
def NAME#"16dd" : MxSROp_DD<mnemonic, MxDRD16, node>;
def NAME#"32dd" : MxSROp_DD<mnemonic, MxDRD32, node>;

def NAME#"8di"  : MxSROp_DI<mnemonic, MxDRD8,  node>;
def NAME#"16di" : MxSROp_DI<mnemonic, MxDRD16, node>;
def NAME#"32di" : MxSROp_DI<mnemonic, MxDRD32, node>;

} // $src = $dst
} // Defs = [CCR]

} // MxBiArOp_RF

defm SHL : MxSROp<"lsl", shl>;
defm ASR : MxSROp<"asr", srl>;
defm LSR : MxSROp<"lsr", sra>;

defm ROL : MxSROp<"rol", rotl>;
defm ROR : MxSROp<"ror", rotr>;
