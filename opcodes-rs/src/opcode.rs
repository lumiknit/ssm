// Arg Types
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum ArgType {
  Int(u32),
  Uint(u32),
  Float(u32),
  Bytes(u32),
  Magic(u32),
  Offset(u32),
  Jmptbl(u32),
}

#[derive(Debug, PartialEq, Clone)]
pub enum ArgVal {
  Int(i32),
  Uint(u32),
  Float(f32),
  Bytes(Vec<u8>),
  Magic(u32),
  Offset(u32),
  Jmptbl(Vec<u32>),
}




// --- Opcode & type listing

// Types

pub static I8: &'static ArgType = &ArgType::Int(8);
pub static I16: &'static ArgType = &ArgType::Int(16);
pub static I32: &'static ArgType = &ArgType::Int(32);
pub static U8: &'static ArgType = &ArgType::Uint(8);
pub static U16: &'static ArgType = &ArgType::Uint(16);
pub static U32: &'static ArgType = &ArgType::Uint(32);
pub static F32: &'static ArgType = &ArgType::Float(32);
pub static B16: &'static ArgType = &ArgType::Bytes(16);
pub static B32: &'static ArgType = &ArgType::Bytes(32);
pub static M16: &'static ArgType = &ArgType::Magic(16);
pub static O32: &'static ArgType = &ArgType::Offset(32);
pub static J32: &'static ArgType = &ArgType::Jmptbl(32);

// Magics

pub const MAGIC: &'static [&str] = &[
  "NOP",
  "HALT",
  "NEWVM",
  "NEWPROCESS",
  "VMSELF",
  "VMPARENT",
  "EVAL",
  "HALTED",
  "SENDMSG",
  "RECVMSG",
  "FOPEN",
  "FCLOSE",
  "FFLUSH",
  "FREAD",
  "FWRITE",
  "FTELL",
  "FSEEK",
  "FEOF",
  "STDREAD",
  "STDWRITE",
  "STDERROR",
  "REMOVE",
  "RENAME",
  "TMPFILE",
  "READFILE",
  "WRITEFILE",
  "MALLOC",
  "FREE",
  "SRAND",
  "RAND",
  "ARG",
  "ENV",
  "EXIT",
  "SYSTEM",
  "PI",
  "E",
  "ABS",
  "SIN",
  "COS",
  "TAN",
  "ASIN",
  "ACOS",
  "ATAN",
  "ATAN2",
  "EXP",
  "LOG",
  "LOG10",
  "LN",
  "MODF",
  "POW",
  "SQRT",
  "CEIL",
  "FLOOR",
  "FABS",
  "FMOD",
  "CLOCK",
  "TIME",
  "CWD",
  "ISDIR",
  "ISFILE",
  "MKDIR",
  "RMDIR",
  "CHDIR",
  "FILES",
  "FFILOAD",
];

// Opcodes

pub static OPCODES: &'static [(&str, &[&ArgType])] = &[
  ("NOP", &[]),
  // Header
  ("HEADER", &[U32, U32, U32]),
  ("HALT", &[]),
  // Stack
  ("POP", &[U16]),
  ("PUSH", &[U16]),
  ("PUSHBP", &[U16]),
  ("PUSHAP", &[I16]),
  ("POPSET", &[U16]),
  // Literal
  ("PUSHI", &[I32]),
  ("PUSHF", &[F32]),
  // Function
  ("PUSHFN", &[O32]),
  // Global
  ("PUSHGLOBAL", &[U32]),
  ("POPSETGLOBAL", &[U32]),
  // Tuple
  ("PUSHISLONG", &[U16]),
  // Short tuple
  ("TUP", &[U16, U16]),
  ("TAG", &[U8]),
  ("PUSHTAG", &[U16]),
  ("PUSHLEN", &[U16]),
  ("PUSHELEM", &[U16, U16]),
  // Long tuple
  ("LONG", &[B32]),
  ("PACK", &[U32]),
  ("SETBYTE", &[U16, U16]),
  ("PUSHLONGLEN", &[U16]),
  ("PUSHBYTE", &[U16]),
  ("JOIN", &[]),
  ("SUBLONG", &[]),
  ("LONGCMP", &[]),
  // Call
  ("APP", &[U32]),
  ("RET", &[U32]),
  ("RETAPP", &[U32]),
  // Int Arithmetic
  ("INTADD", &[]),
  ("INTSUB", &[]),
  ("INTMUL", &[]),
  ("UINTMUL", &[]),
  ("INTDIV", &[]),
  ("UINTDIV", &[]),
  ("INTMOD", &[]),
  ("UINTMOD", &[]),
  ("INTUNM", &[]),
  ("INTSHL", &[]),
  ("INTSHR", &[]),
  ("UINTSHR", &[]),
  ("INTAND", &[]),
  ("INTOR", &[]),
  ("INTXOR", &[]),
  ("INTNEG", &[]),
  ("INTLT", &[]),
  ("INTLE", &[]),
  // Float Arithmetic
  ("FLOATADD", &[]),
  ("FLOATSUB", &[]),
  ("FLOATMUL", &[]),
  ("FLOATDIV", &[]),
  ("FLOATUNM", &[]),
  ("FLOATLT", &[]),
  ("FLOATLE", &[]),
  // Comparison
  ("EQ", &[]),
  ("NE", &[]),
  // Branch
  ("JMP", &[O32]),
  ("BEQ", &[O32]),
  ("BNE", &[O32]),
  ("BTAG", &[U16, O32]),
  ("JTAG", &[J32]),
  // Magic
  ("MAGIC", &[M16]),
  // Literal Marker
  ("XFN", &[U16, U32])
];