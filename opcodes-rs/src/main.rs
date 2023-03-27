pub mod opcode;
pub mod cgen;

const cmd: Command = Command::CGen;

enum Command {
  Nop,
  CGen,
}



fn main() {
  match cmd {
    Command::Nop => (),
    Command::CGen => {
      cgen::gen_c_files();
    },
    _ => ()
  }
}
