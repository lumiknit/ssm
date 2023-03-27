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
  pub fn size(&self) -> usize {
    match self {
      &ArgType::Int(size) => size as usize,
      &ArgType::Uint(size) => size as usize,
      &ArgType::Float(size) => size as usize,
      &ArgType::Bytes(size) => size as usize,
      &ArgType::Magic(size) => size as usize,
      &ArgType::Offset(size) => size as usize,
      &ArgType::Jmptbl(size) => size as usize,
    }
  }

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

  pub fn pack(&self, val: &ArgVal) -> Option<Vec<u8>> {
    match (self, val) {
      (&ArgType::Int(size), &ArgVal::Int(v)) =>
        match size {
          1 => Some(vec![v as i8 as u8]),
          2 => Some(i16::to_le_bytes(v as i16).to_vec()),
          4 => Some(i32::to_le_bytes(v).to_vec()),
          _ => None
        },
      (&ArgType::Uint(size), &ArgVal::Uint(v)) =>
        match size {
          1 => Some(vec![v as u8]),
          2 => Some(u16::to_le_bytes(v as u16).to_vec()),
          4 => Some(u32::to_le_bytes(v).to_vec()),
          _ => None
        },
      (&ArgType::Float(size), &ArgVal::Float(v)) =>
        match size {
          4 => Some(f32::to_le_bytes(v).to_vec()),
          _ => None
        },
      (&ArgType::Bytes(size), &ArgVal::Bytes(ref v)) => {
        let len = v.len();
        let mut bytes = match size {
          2 => u16::to_le_bytes(len as u16).to_vec(),
          4 => u32::to_le_bytes(len as u32).to_vec(),
          _ => return None
        };
        bytes.extend_from_slice(v);
        Some(bytes)
      },
      (&ArgType::Magic(size), &ArgVal::Magic(v)) =>
        match size {
          1 => Some(vec![v as u8]),
          2 => Some(u16::to_le_bytes(v as u16).to_vec()),
          _ => None
        },
      (&ArgType::Offset(size), &ArgVal::Offset(v)) =>
        match size {
          2 => Some(u16::to_le_bytes(v as u16).to_vec()),
          4 => Some(u32::to_le_bytes(v).to_vec()),
          _ => None
        },
      (&ArgType::Jmptbl(size), &ArgVal::Jmptbl(ref v)) => {
        let mut bytes = u16::to_le_bytes(v.len() as u16).to_vec();
        for o in v {
          match size {
            2 => bytes.extend_from_slice(&u16::to_le_bytes(*o as u16)),
            4 => bytes.extend_from_slice(&u32::to_le_bytes(*o)),
            _ => return None
          }
        }
        Some(bytes)
      },
      _ => None
    }
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

pub fn magic_name(idx: u16) -> Option<&'static str> {
  if idx as usize >= MAGIC.len() {
    None
  } else {
    Some(MAGIC[idx as usize])
  }
}

pub fn magic_idx(name: String) -> Option<u16> {
  for (i, m) in MAGIC.iter().enumerate() {
    if m == &name {
      return Some(i as u16);
    }
  }
  None
}

// Opcodes

type Opcode<'r> = (&'r str, &'r [&'r ArgType]);
pub static OPCODES: &'static [Opcode<'static>] = &[
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

pub fn opcode(idx: u16) -> Option<Opcode<'static>> {
  if idx as usize >= OPCODES.len() {
    None
  } else {
    Some(OPCODES[idx as usize])
  }
}

pub fn opcode_idx(name: String) -> Option<u16> {
  for (i, o) in OPCODES.iter().enumerate() {
    if o.0 == name {
      return Some(i as u16);
    }
  }
  None
}