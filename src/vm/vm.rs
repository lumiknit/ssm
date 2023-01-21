use crate::gc::*;
use crate::vm::code;

pub struct VM {
    // Memory
    mem: Mem,
    codes: code::Codes,

    // Registers
    acc: val::Val,
    pc: usize,
}

impl VM {
    pub fn run() -> Result<(), String> {
        unimplemented!();
    }
}

// VM configurations (builder)
pub struct VMConfig {
    // Memory

    // Initial size (in words) of global and stack
    global_initial_vals: val::Uptr,
    stack_initial_vals: val::Uptr,

    // Initial size (in bytes) of minor and major pools
    minor_pool_initial_bytes: usize,
    major_pool_initial_bytes: usize,
}

impl VMConfig {
    pub fn build(self) -> VM {
        let mem = Mem::new(
            self.global_initial_vals,
            self.stack_initial_vals,
            self.minor_pool_initial_bytes,
            self.major_pool_initial_bytes,
        );
        let codes = code::Codes::new();
        VM {
            mem: mem,
            codes: codes,
            acc: val::Val(0),
            pc: 0,
        }
    }
}