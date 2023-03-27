use std::fs::File;
use std::io::prelude::*;

use crate::opcode::*;

fn gen_opcodes_hd() -> std::io::Result<()> {
  let mut file = File::create("../include/ssm_ops.h")?;
  writeln!(file, "// Generated by opcode-rs")?;
  writeln!(file, "#include <stdint.h>")?;
  for (idx, op) in OPCODES.into_iter().enumerate() {
    writeln!(file, "#define SSM_OP_{} ((ssmOp){});", op.0, idx)?;
  }
  Ok(())
}

fn gen_switch() -> std::io::Result<()> {
  let mut file = File::create("sw.c")?;
  writeln!(file, "// Generated by opcode-rs")?;
  for op in OPCODES.into_iter() {
    let mut read = 1;
    writeln!(file, "OP({}): {{", op.0)?;
    for (idx, arg) in op.1.into_iter().enumerate() {
      match arg {
        ArgType::Uint(size) => {
          writeln!(file, "  uint{}_t a{} = SSM_READ_U{}(ip + {});",
            size * 8, idx, size * 8, read)?;
          read += size;
        },
        ArgType::Int(size) => {
          writeln!(file, "  int{}_t a{} = SSM_READ_I{}(ip + {});",
            size * 8, idx, size * 8, read)?;
          read += size;
        },
        ArgType::Float(4) => {
          writeln!(file, "  float a{} = SSM_READ_F{}(ip + {});",
            idx, 32, read)?;
          read += 4;
        },
        ArgType::Magic(size) => {
          writeln!(file, "  uint{}_t a{} = SSM_READ_U{}(ip + {});",
            size * 8, idx, size * 8, read)?;
          read += size;
        },
        ArgType::Offset(size) => {
          writeln!(file, "  int{}_t a{} = SSM_READ_I{}(ip + {});",
            size * 8, idx, size * 8, read)?;
          read += size;
        },
        _ => ()
      }
    }
    writeln!(file, "}} NEXT({});", read)?;
  }
  Ok(())
}

fn gen_jmptbl() -> std::io::Result<()>{
  let mut file = File::create("jmptbl.c")?;
  writeln!(file, "// Generated by opcode-rs")?;
  for op in OPCODES.into_iter() {
    writeln!(file, "&&L_op_{},", op.0)?;
  }
  Ok(())
}

pub fn gen_c_files() {
  if let Err(e) = gen_opcodes_hd() {
    println!("Error: {}", e);
  }
  if let Err(e) = gen_switch() {
    println!("Error: {}", e);
  }
  if let Err(e) = gen_jmptbl() {
    println!("Error: {}", e);
  }
}