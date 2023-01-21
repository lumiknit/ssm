use ssm::gc::*;

#[test]
fn create_mem() {
    let _mem = mem::Mem::new(
        128,
        128,
        1024,
        2048,
    );
}