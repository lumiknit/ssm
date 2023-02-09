use ssm::gc::val::*;
use ssm::gc::*;

#[test]
fn create_mem() {
    let _mem = Mem::new(128, 128, 1024);
}

#[test]
fn allocate_short() {
    let mut mem = Mem::new(128, 128, 1024);
    let tup = mem.alloc_short(4, 12);
    let hd = tup.header();
    assert!(!hd.is_long());
    assert!(hd.is_white());
    assert_eq!(hd.tag(), 12);
}

#[test]
fn allocate_shorts() {
    let mut mem = Mem::new(128, 128, 1024);
    let tup1 = mem.alloc_short(4, 1);
    let tup2 = mem.alloc_short(4, 2);
    for i in 0..4 {
        tup1.set_val(i, Val(i + 1));
        tup2.set_val(i, Val(i + 42));
    }
    assert!(!tup1.is_long());
    assert!(!tup2.is_long());
    assert_eq!(tup1.len(), 4);
    assert_eq!(tup2.len(), 4);
    assert_eq!(tup1.tag(), 1);
    assert_eq!(tup2.tag(), 2);
    for i in 0..4 {
        assert_eq!(tup1.val(i), Val(i + 1));
        assert_eq!(tup2.val(i), Val(i + 42));
    }
    unsafe {
        assert_eq!(tup2.0.add(Tup::words_from_words(4) as usize), tup1.0);
        assert!(mem.minor_pool.own(tup1.0));
        assert!(mem.minor_pool.own(tup2.0));
    }
}

#[test]
fn minor_gc_00() {
    let word_size = std::mem::size_of::<usize>();
    let mut mem = Mem::new(16, 16, 32 * word_size);
    mem.alloc_short(4, 1);
    assert_eq!(mem.minor_pool.left, 27);
    mem.alloc_short(4, 1);
    assert_eq!(mem.minor_pool.left, 22);
    mem.alloc_short(4, 1);
    assert_eq!(mem.minor_pool.left, 17);
    mem.alloc_short(10, 1);
    assert_eq!(mem.minor_pool.left, 6);
    // GC invoked
    mem.alloc_short(10, 1);
    assert_eq!(mem.minor_pool.left, 21);
}

#[test]
fn minor_gc_01() {
    let word_size = std::mem::size_of::<usize>();
    let mut mem = Mem::new(16, 16, 32 * word_size);
    // Create a garbage
    mem.alloc_short(4, 41);
    assert_eq!(mem.minor_pool.left, 27);
    // Create a non-garbate
    let v = mem.alloc_short(4, 42).to_val();
    assert_eq!(mem.minor_pool.left, 22);
    mem.stack.push(v.0);
    // Create one more garbage
    mem.alloc_short(4, 43);
    assert_eq!(mem.minor_pool.left, 17);
    // Create one more non-garbage
    let w = mem.alloc_short(4, 44).to_val();
    assert_eq!(mem.minor_pool.left, 12);
    mem.stack.push(w.0);
    // GC invoked
    mem.collect_minor();
    assert_eq!(mem.minor_pool.left, 32);
    //assert_eq!(mem.pools[1].left, 118);
    for i in 0..2 {
        let tup = Tup::from_val(Val(mem.stack[i]));
        assert!(!mem.minor_pool.own(Val(mem.stack[i]).to_gc_ptr()));
        assert!(tup.tag() == 42 + 2 * i);
        assert!(tup.header().is_white());
        assert!(!tup.is_long());
        assert!(tup.len() == 4);
    }
    unsafe {
        let p0 = Val(mem.stack[0]).to_gc_ptr::<usize>();
        let p1 = Val(mem.stack[1]).to_gc_ptr::<usize>();
        assert!(p0.add(5) == p1 || p1.add(5) == p0);
    }
}

#[test]
fn minor_gc_02() {
    let word_size = std::mem::size_of::<usize>();
    let mut mem = Mem::new(16, 16, 32 * word_size);
    for _i in 0..100 {
        mem.alloc_short(4, 0);
        mem.alloc_long(48);
    }
    // Create non-garbage
    let v = mem.alloc_short(4, 42).to_val();
    mem.stack.push(v.0);
    for i in 0..4 {
        Tup::from_val(v).set_val(i, Val(i + 123));
    }
    // Create lots of garbage
    for _i in 0..10 {
        mem.alloc_short(4, 0);
        mem.alloc_long(41);
    }
    // Create non-garbatge
    let w = mem.alloc_long(56).to_val();
    mem.stack.push(w.0);
    let msg = "Hello, World!";
    unsafe {
        for i in 0..msg.len() {
            Tup::from_val(w).bytes().add(i).write(msg.as_bytes()[i]);
        }
    }
    // Create lots of garbage
    for _i in 0..10 {
        mem.alloc_short(4, 0);
        mem.alloc_long(46);
    }
    // Check first value
    let tup_v = Tup::from_val(Val(mem.stack[0]));
    assert!(!tup_v.is_long());
    assert!(tup_v.tag() == 42);
    for i in 0..4 {
        assert!(tup_v.val(i) == Val(i + 123));
    }
    // Check second value
    let tup_w = Tup::from_val(Val(mem.stack[0]));
    assert!(tup_w.is_long());
    for i in 0..msg.len() {
        assert_eq!(tup_w.byte_at(i), msg.as_bytes()[i]);
    }
}

#[test]
fn major_gc_00() {
    let word_size = std::mem::size_of::<usize>();
    let mut mem = Mem::new(16, 16, 32 * word_size);
    // Create a garbage
    mem.alloc_short(4, 41);
    assert_eq!(mem.minor_pool.left, 27);
    // Create a non-garbate
    let v = mem.alloc_short(4, 42).to_val();
    assert_eq!(mem.minor_pool.left, 22);
    mem.stack.push(v.0);
    // Create one more garbage
    mem.alloc_short(4, 43);
    assert_eq!(mem.minor_pool.left, 17);
    // Create one more non-garbage
    let w = mem.alloc_short(4, 44).to_val();
    assert_eq!(mem.minor_pool.left, 12);
    mem.global.push(w.0);
    // GC invoked
    mem.collect_minor();
    assert_eq!(mem.minor_pool.left, 32);
    // Clean-up all stacks
    mem.stack.pop();
    mem.global.pop();
    // Run minor
    mem.collect_minor();
    assert_eq!(mem.minor_pool.left, 32);
    // Run Major
    mem.collect_major();
    assert_eq!(mem.minor_pool.left, 32);
}
