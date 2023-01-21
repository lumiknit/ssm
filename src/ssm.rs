pub mod gc;
pub mod vm;

fn main() {
    let mut vm = vm::VMConfig {
        global_initial_vals: 1024,
        stack_initial_vals: 1024 * 128,
        minor_pool_initial_bytes: 1024 * 1024,
        major_pool_initial_bytes: 1024 * 1024 * 4,
    }.build();

    if let Err(err) = vm.run() {
        println!("Error during vm.run():\n{}", err);
    }
}
