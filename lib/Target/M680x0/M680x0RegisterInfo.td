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
  def MxSubRegIndex8Lo  : SubRegIndex<8, 0>;
  def MxSubRegIndex16Lo : SubRegIndex<16, 0>;
}

// Data Registers
class MxDataReg<string n, bits<16> enc, list<Register> subregs = []>
    : MxReg<n, enc, subregs>;

// Generate Data registers and theirs smaller variants
foreach Index = 0-7 in {
  def "BD"#Index : MxDataReg<"d"#Index, #Index>;

  let SubRegIndices = [ MxSubRegIndex8Lo ] in {
    def "WD"#Index
      : MxDataReg<"d"#Index, #Index, [ !cast<Register>("BD"#Index) ]>,
        DwarfRegNum<[Index]>;
  }

  let SubRegIndices = [ MxSubRegIndex16Lo ] in {
    def "D"#Index
      : MxDataReg<"d"#Index, #Index, [ !cast<Register>("WD"#Index) ]>,
        DwarfRegNum<[Index]>;
  }

} // foreach

// Address Registers
class MxAddressReg<string n, bits<16> enc, list<Register> subregs = []>
    : MxReg<n, enc, subregs>;

// Generate Address registers and theirs smaller variants
foreach Index = 0-7 in {
  def "WA"#Index
     : MxDataReg<"a"#Index, #Index>,
       DwarfRegNum<[!add(8,Index)]>;

  let SubRegIndices = [ MxSubRegIndex16Lo ] in {
    def "A"#Index
       : MxAddressReg<"a"#Index, #Index, [ !cast<Register>("WA"#Index) ]>,
         DwarfRegNum<[!add(8,Index)]>;
  }
}

// Alias Registers
class MxAliasReg<string n, MxReg reg>
    : MxReg<n, reg.HWEncoding> {
  let Aliases = [reg];
}

// def BP  : MxAliasReg<"bp",  A5>;
// def FP  : MxAliasReg<"fp",  A6>;
def SP  : MxAliasReg<"sp",  A7>;

// def USP : MxAliasReg<"usp", A7>;
// def SSP : MxAliasReg<"ssp", A7>;
// def ISP : MxAliasReg<"isp", A7>;

// TODO get rid of "pseudo" registers and use just MxReg variants and use
// HWEncoding's other 13 bits to encode type(and potentially other info) of
// register.

// Pseudo Registers
class MxPseudoReg<string n, list<Register> subregs = []>
    : MxReg<n, 0, subregs>;

def CCR : MxPseudoReg<"ccr">;

let SubRegIndices = [MxSubRegIndex8Lo] in {
  def SR : MxPseudoReg<"sr", [CCR]>;
}

def PC : MxPseudoReg<"pc">;

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

class MxRegClass<list<ValueType> regTypes, int alignment, dag regList>
    : RegisterClass<"M680x0", regTypes, alignment, regList>;

// Data Registers
def DR8  : MxRegClass<[i8],  16, (sequence "BD%u", 0, 7)>;
def DR16 : MxRegClass<[i16], 16, (sequence "WD%u", 0, 7)>;
def DR32 : MxRegClass<[i32], 32, (sequence "D%u",  0, 7)>;

// Address Registers
def AR16 : MxRegClass<[i16], 16, (sequence "WA%u", 0, 6)>;
def AR32 : MxRegClass<[i32], 32, (add (sequence "A%u", 0, 6), SP)>;

// Index Register Classes
def XR16 : MxRegClass<[i16], 16, (add DR16, AR16)>;
def XR32 : MxRegClass<[i32], 32, (add DR32, AR32)>;

// Control Registers
/* let CopyCost = -1, isAllocatable = 0 in { */
  def CCRC : MxRegClass<[i8],  16, (add CCR)>;
  def SRC  : MxRegClass<[i16], 16, (add SR)>;
  def PCC  : MxRegClass<[i32], 32, (add PC)>;
  def SPC  : MxRegClass<[i32], 32, (add A7)>;
/* } */

// Register used with tail call
def DR16_TC : MxRegClass<[i16], 16, (add D0, D1)>;
def DR32_TC : MxRegClass<[i32], 32, (add D0, D1)>;

def AR16_TC : MxRegClass<[i16], 16, (add A0, A1)>;
def AR32_TC : MxRegClass<[i32], 32, (add A0, A1)>;

def XR16_TC : MxRegClass<[i16], 16, (add DR16_TC, AR16_TC)>;
def XR32_TC : MxRegClass<[i32], 32, (add DR32_TC, AR32_TC)>;
