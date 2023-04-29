
/* Instruction
 * each instruction is saved in 32 bits in little endian
 * - 8 bit (0x000000ff) is opcode
 * - 24 bit (0xffffff00) is arguments
 */

pub struct Instr(pub u32);

/* Opmode is a structure of 24-bit arguments
 * This is used when vm dispatch opcodess
 */
pub enum OpMode {
  U, /* u24 */
  I, /* i24 */
  Uu, /* u8, u16 */
}

/* OpType is a kind of opcode.
 * and denote how each arguments are used.
 * This is just for verify opcode chunk */
pub enum OpType {
  U, /* u24 */
  G, /* u24, global index */
  M, /* u24, magic */
  I, /* i24 */
  O, /* i24, offset from ip */
  Fn, /* i24, function, offset from ip */
  Uu, /* u8, u16 */
}

/* OpCode
 * It must be ordered by OpType */
pub enum OpCode {
  // -- U
  Nop,
  Pop,
  PopSet,
  Push,
  PushBP,
  PushAP,
  ImmUint,
  IsLong,
  Tag,
  Len,
  ElemL,
  LongLen,
  Byte,
  SetByte,
  Join,
  SubLong,
  LongCmp,
  App,
  Ret,
  RetApp,
  IntAdd,
  Eq,
  BTag,
  XFn,
  ExtStack,
  // -- G
  Global,
  SetGlobal,
  // -- M
  Magic,
  // -- I
  ImmInt,
  // -- O
  LitInt,
  LitFlt,
  Long,
  Jmp,
  Bzero,
  JTag,
  // -- Fn
  Fn,
  // -- Uu
  Tup,
  Tupl,
  Elem,
}

pub const OP_U_FIRST: u32 = OpCode::Nop as u32;
pub const OP_G_FIRST: u32 = OpCode::Global as u32;
pub const OP_M_FIRST: u32 = OpCode::Magic as u32;
pub const OP_I_FIRST: u32 = OpCode::ImmInt as u32;
pub const OP_O_FIRST: u32 = OpCode::LitInt as u32;
pub const OP_FN_FIRST: u32 = OpCode::Fn as u32;
pub const OP_UU_FIRST: u32 = OpCode::Tup as u32;
pub const OP_LAST: u32 = OpCode::Elem as u32;

impl OpCode {
  #[inline(always)]
  pub fn typ(self) -> OpType {
    let op = self as u32;
    if op < OP_G_FIRST {
      OpType::U
    } else if op < OP_M_FIRST {
      OpType::G
    } else if op < OP_I_FIRST {
      OpType::M
    } else if op < OP_O_FIRST {
      OpType::I
    } else if op < OP_FN_FIRST {
      OpType::O
    } else if op < OP_UU_FIRST {
      OpType::Fn
    } else {
      OpType::Uu
    }
  }
}



impl Instr {
  #[inline(always)]
  pub fn from_bytes(bytes: &[u8;4]) -> Self {
    Self(u32::from_le_bytes(*bytes))
  }

  #[inline(always)]
  pub fn op(&self) -> u32 {
    self.0 & 0xff
  }

  #[inline(always)]
  pub fn args_u(&self) -> u32 {
    self.0 >> 8
  }

  #[inline(always)]
  pub fn args_i(&self) -> i32 {
    self.0 as i32 >> 8
  }

  #[inline(always)]
  pub fn args_uu(&self) -> (u8, u16) {
    let first: u8 = (self.0 >> 8) as u8;
    let second: u16 = (self.0 >> 16) as u16;
    (first, second)
  }
}


pub enum OpIdx {
  
}

pub enum Op {
  Nop,

}

pub enum Magic {
  Nop,
  Ptop,
  Halt,
  Newvm,
  Newprocess,
  Vmself,
  Vmparent,
  Dup,
  Globalc,
  Execute,
  Halted,
  Sendmsg,
  Hasmsg,
  Recvmsg,
  Eval,
  Fopen,
  Fclose,
  Fflush,
  Fread,
  Fwrite,
  Ftell,
  Fseek,
  Feof,
  Stdread,
  Stdwrite,
  Stderror,
  Remove,
  Rename,
  Tmpfile,
  Readfile,
  Writefile,
  Malloc,
  Free,
  Srand,
  Rand,
  Arg,
  Env,
  Exit,
  System,
  Pi,
  E,
  Abs,
  Sin,
  Cos,
  Tan,
  Asin,
  Acos,
  Atan,
  Atan2,
  Exp,
  Log,
  Log10,
  Modf,
  Pow,
  Sqrt,
  Ceil,
  Floor,
  Fabs,
  Fmod,
  Clock,
  Time,
  Cwd,
  Isdir,
  Isfile,
  Mkdir,
  Rmdir,
  Chdir,
  Files,
  Joinpath,
  Ffiload,
  Os,
  Arch,
  Endian,
  Version,
}