pub mod gc;
pub mod vm;

fn main() {
    let mut vm = vm::VMConfig {
        global_initial_words: 128,
        stack_initial_words: 1024 * 128,
        minor_pool_bytes: 1024 * 1024, // 1MB
        major_gc_threshold_percent: 120,
    }
    .build();

    if let Err(err) = vm.run() {
        println!("Error during vm.run():\n{}", err);
    }
}
