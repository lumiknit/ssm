pub mod code;

use crate::gc::*;

#[allow(dead_code)]
pub struct VM {
    // Memory
    mem: Mem,
    codes: code::Codes,

    // Registers
    acc: val::Val,
    pc: usize,
}

impl VM {
    pub fn run(&mut self) -> Result<(), String> {
        unimplemented!();
    }
}

// VM configurations (builder)
pub struct VMConfig {
    // Memory

    // Initial size (in words) of global and stack
    pub global_initial_vals: usize,
    pub stack_initial_vals: usize,

    // Initial size (in bytes) of minor and major pools
    pub minor_pool_initial_bytes: usize,
    pub major_pool_initial_bytes: usize,
}

impl VMConfig {
    pub fn build(self) -> VM {
        let mem = Mem::new(
            self.global_initial_vals,
            self.stack_initial_vals,
            self.minor_pool_initial_bytes,
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
