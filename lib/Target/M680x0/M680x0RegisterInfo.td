//== M680x0RegisterInfo.td - Target definition file for the M680x0 tablegen ==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the M680x0 Register file, defining the registers aliases
// between the registers, and the register classes built out of the registers.
//
//===----------------------------------------------------------------------===//

class MxReg<string n, bits<16> enc, list<Register> subregs = []>
    : Register<n> {
  let Namespace = "M680x0";
  let HWEncoding = enc;
  let SubRegs    = subregs;
}

// Subregister indices.
let Namespace = "M680x0" in {
  def SubRegIndex8Lo : SubRegIndex<8, 0>;
}

// Data Registers
class MxDataReg<string n, bits<16> enc> : MxReg<n, enc>;

foreach  Index = 0-7 in {
  def D#Index : MxDataReg<"d"#Index, #Index>;
}

// Address Registers
class MxAddressReg<string n, bits<16> enc> : MxReg<n, enc>;

foreach  Index = 0-7 in {
  def A#Index : MxAddressReg<"a"#Index, #Index>;
}

// Alias Registers
class MxAliasReg<string n, MxReg reg>
    : MxReg<n, reg.HWEncoding, reg.SubRegs> {
  let Aliases = [reg];
}

def BP  : MxAliasReg<"bp",  A5>;
def FP  : MxAliasReg<"fp",  A6>;
def SP  : MxAliasReg<"sp",  A7>;

def USP : MxAliasReg<"usp", A7>;
def SSP : MxAliasReg<"ssp", A7>;
def ISP : MxAliasReg<"isp", A7>;

// TODO get rid of "pseudo" registers and use just MxReg variants and use
// HWEncoding's other 13 bits to encode type(and potentially other info) of
// register.

// Pseudo Registers
class MxPseudoReg<string n, list<Register> subregs = []>
    : MxReg<n, 0, subregs>;

def CCR : MxPseudoReg<"ccr">;

let SubRegIndices = [SubRegIndex8Lo] in {
  def SR : MxPseudoReg<"sr", [CCR]>;
}

def PC : MxPseudoReg<"pc">;

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

class MxRegClass<list<ValueType> regTypes, int alignment, dag regList>
    : RegisterClass<"M680x0", regTypes, alignment, regList>;

// Data Registers
def DR8  : MxRegClass<[i8],  16, (sequence "D%u", 0, 7)>;
def DR16 : MxRegClass<[i16], 16, (sequence "D%u", 0, 7)>;
def DR32 : MxRegClass<[i32], 32, (sequence "D%u", 0, 7)>;

// Address Registers
// A7 is not part of address class due to its strict use as Stack Pointer
def AR16 : MxRegClass<[i16], 16, (sequence "A%u", 0, 7)>;
def AR32 : MxRegClass<[i32], 32, (sequence "A%u", 0, 7)>;

// Index Register Classes
def XR16 : MxRegClass<[i16], 16, (add DR16, AR16)>;
def XR32 : MxRegClass<[i32], 32, (add DR32, AR32)>;

// Control Registers
/* let CopyCost = -1, isAllocatable = 0 in { */
  def CR8  : MxRegClass<[i8],  16, (add CCR)>;
  def CR16 : MxRegClass<[i16], 16, (add SR)>;
  def CPC  : MxRegClass<[i32], 32, (add PC)>;
  def CSP  : MxRegClass<[i32], 32, (add SP)>;
/* } */

// Register used with tail call
def DR16_TC : MxRegClass<[i16], 16, (add D0, D1)>;
def DR32_TC : MxRegClass<[i32], 32, (add D0, D1)>;

def AR16_TC : MxRegClass<[i16], 16, (add A0, A1)>;
def AR32_TC : MxRegClass<[i32], 32, (add A0, A1)>;

def XR16_TC : MxRegClass<[i16], 16, (add DR16_TC, AR16_TC)>;
def XR32_TC : MxRegClass<[i32], 32, (add DR32_TC, AR32_TC)>;
