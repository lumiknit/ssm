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

fn clone_bytes<const L: usize>(a: &[u8]) -> Option<[u8; L]> {
  if a.len() < L {
    None
  } else {
    let mut b: [u8; L] = [0; L];
    b.clone_from_slice(a);
    Some(b)
  }
}

impl ArgType {
  pub fn unpack(&self, bytes: &[u8]) -> Option<(usize, ArgVal)> {
    match self {
      &ArgType::Int(size) =>
        match size {
          1 => Some((1, ArgVal::Int(bytes[0] as i8 as i32))),
          2 => clone_bytes(bytes)
            .map(|b| (2, ArgVal::Int(i16::from_le_bytes(b) as i32))),
          4 => clone_bytes(bytes)
            .map(|b| (4, ArgVal::Int(i32::from_le_bytes(b)))),
          _ => None
        },
      &ArgType::Uint(size) =>
        match size {
          1 => Some((1, ArgVal::Uint(bytes[0] as u32))),
          2 => clone_bytes(bytes)
            .map(|b| (2, ArgVal::Uint(u16::from_le_bytes(b) as u32))),
          4 => clone_bytes(bytes)
            .map(|b| (4, ArgVal::Uint(u32::from_le_bytes(b)))),
          _ => None
        },
      &ArgType::Float(size) =>
        match size {
          4 => clone_bytes(bytes)
            .map(|b| (4, ArgVal::Float(f32::from_le_bytes(b)))),
          _ => None
        },
      &ArgType::Bytes(size) => {
        let s = size as usize;
        let len = match size {
          2 => u16::from_le_bytes(clone_bytes(&bytes)?) as usize,
          4 => u32::from_le_bytes(clone_bytes(&bytes)?) as usize,
          _ => return None
        };
        if bytes.len() < len + s {
          return None
        }
        Some((len + s, ArgVal::Bytes(bytes[len .. len + s].to_vec())))
      },
      &ArgType::Magic(size) =>
        match size {
          1 => Some((1, ArgVal::Magic(bytes[0] as u32))),
          2 => clone_bytes(bytes)
            .map(|b| (2, ArgVal::Magic(u16::from_le_bytes(b) as u32))),
          _ => None
        },
      &ArgType::Offset(size) =>
        match size {
          2 => clone_bytes(bytes)
            .map(|b| (2, ArgVal::Offset(u16::from_le_bytes(b) as u32))),
          4 => clone_bytes(bytes)
            .map(|b| (4, ArgVal::Offset(u32::from_le_bytes(b)))),
          _ => None
        },
      &ArgType::Jmptbl(size) => {
        let len = u16::from_le_bytes(clone_bytes(&bytes)?) as usize;
        let tbl_size = 2 + len * size as usize;
        if bytes.len() < tbl_size {
          return None
        }
        let mut v = Vec::new();
        for i in 0..len {
          let offset = 2 + i * size as usize;
          let o = match size {
            2 => u16::from_le_bytes(clone_bytes(&bytes[offset..])?) as u32,
            4 => u32::from_le_bytes(clone_bytes(&bytes[offset..])?),
            _ => return None
          };
          v.push(o);
        }
        Some((tbl_size, ArgVal::Jmptbl(v)))
      }
    }
  }

  pub fn pack(&self, val: &ArgVal) -> Vec<u8> {

  }
}

impl ArgVal {
  pub fn check_type(&self, arg_type: &ArgType) -> bool {
    match (self, arg_type) {
      (&ArgVal::Int(_), &ArgType::Int(_)) => true,
      (&ArgVal::Uint(_), &ArgType::Uint(_)) => true,
      (&ArgVal::Float(_), &ArgType::Float(_)) => true,
      (&ArgVal::Bytes(_), &ArgType::Bytes(_)) => true,
      (&ArgVal::Magic(_), &ArgType::Magic(_)) => true,
      (&ArgVal::Offset(_), &ArgType::Offset(_)) => true,
      (&ArgVal::Jmptbl(_), &ArgType::Jmptbl(_)) => true,
      _ => false,
    }
  }
}

// --- Opcode & type listing

// Types

pub static I8: &'static ArgType = &ArgType::Int(1);
pub static I16: &'static ArgType = &ArgType::Int(2);
pub static I32: &'static ArgType = &ArgType::Int(4);
pub static U8: &'static ArgType = &ArgType::Uint(1);
pub static U16: &'static ArgType = &ArgType::Uint(2);
pub static U32: &'static ArgType = &ArgType::Uint(4);
pub static F32: &'static ArgType = &ArgType::Float(4);
pub static B16: &'static ArgType = &ArgType::Bytes(2);
pub static B32: &'static ArgType = &ArgType::Bytes(4);
pub static M16: &'static ArgType = &ArgType::Magic(2);
pub static O32: &'static ArgType = &ArgType::Offset(4);
pub static J32: &'static ArgType = &ArgType::Jmptbl(4);

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
  ("XFN", &[U16, U32]),
  // Header
  ("HEADER", &[U32, U32, U32]),
  ("HALT", &[]),
];