use ssm::gc::val::*;
use ssm::gc::*;

use std::time::*;

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
    assert_eq!(mem.major_allocated, 10);
    for i in 0..2 {
        let tup = Tup::from_val(Val(mem.stack[i]));
        assert!(!mem.minor_pool.own(Val(mem.stack[i]).to_gc_ptr()));
        assert!(tup.tag() == 42 + 2 * i);
        assert!(tup.header().is_white());
        assert!(!tup.is_long());
        assert!(tup.len() == 4);
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
    println!("tup_v = {:?}", Tup::from_val(v).0);
    for i in 0..4 {
        Tup::from_val(v).set_val(i, Val::from_uint(i + 123));
        println!("{}", Tup::from_val(v).val(i));
    }
    println!("B");
    // Create lots of garbage
    for _i in 0..10 {
        mem.alloc_short(4, 0);
        mem.alloc_long(41);
    }
    println!("C");
    // Create non-garbatge
    let w = mem.alloc_long(56).to_val();
    assert!(Tup::from_val(w).is_long());
    mem.stack.push(w.0);
    println!("D");
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
    println!("tup_v = {:?}", tup_v.0);
    assert!(!tup_v.is_long());
    assert!(tup_v.tag() == 42);
    for i in 0..4 {
        println!("{}", tup_v.val(i));
        assert!(tup_v.val(i) == Val::from_uint(i + 123));
    }
    // Check second value
    let tup_w = Tup::from_val(Val(mem.stack[1]));
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
    assert_eq!(mem.major_allocated, 10);
    // Clean-up all stacks
    mem.stack.pop();
    mem.global.pop();
    // Run minor
    mem.collect_minor();
    assert_eq!(mem.minor_pool.left, 32);
    assert_eq!(mem.major_allocated, 10);
    // Run Major
    mem.collect_major();
    assert_eq!(mem.minor_pool.left, 32);
    assert_eq!(mem.major_allocated, 0);
}

#[test]
fn major_gc_01() {
    let word_size = std::mem::size_of::<usize>();
    let mut mem = Mem::new(16, 16, 16 * word_size);
    // Create a large garbage in major pool;
    mem.alloc_long(word_size * 20);
    assert_eq!(mem.minor_pool.left, 16);
    assert_eq!(mem.major_allocated, 21);
    // Create a large garbage in major pool;
    mem.alloc_long(word_size * 100);
    assert_eq!(mem.minor_pool.left, 16);
    assert_eq!(mem.major_allocated, 122);
    // Create a short garbage
    mem.alloc_short(15, 1);
    assert_eq!(mem.minor_pool.left, 0);
    assert_eq!(mem.major_allocated, 122);
    // Create 
    mem.alloc_short(15, 1);
    assert_eq!(mem.minor_pool.left, 0);
    assert_eq!(mem.major_allocated, 0);
    // Create 
    mem.alloc_short(7, 1);
    assert_eq!(mem.minor_pool.left, 8);
    assert_eq!(mem.major_allocated, 0);
}

#[test]
fn gc_boom_00() {
    let word_size = std::mem::size_of::<usize>();
    let mut mem = Mem::new(16, 16, 256 * word_size);
    mem.stack.push(Val::from_uint(42).0);
    mem.stack.push(Val::from_uint(42).0);
    mem.stack.push(Val::from_uint(42).0);
    mem.stack.push(Val::from_uint(42).0);
    mem.stack.push(Val::from_uint(42).0);
    let v = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap().as_secs();
    let z = i32::MAX as u64;
    let mut acc = 0;
    for _i in 0..10000000 {
        acc = (acc + v) % z;
        match acc % 3 {
            0 => {
                // Just create a trash
                let tup = mem.alloc_short(4, 0);
                for i in 0..4 {
                    tup.set_val(i, Val::from_uint(0))
                }
            },
            1 => {
                // Create a non-garbage
                let tup = mem.alloc_short(5, 0);
                for i in 0..5 {
                    tup.set_val(i, Val::from_uint(0))
                }
                mem.stack[(acc % 5) as usize] = tup.to_val().0;
            },
            _ => {
                // Create a non-garbage and insert into rand value
                let tup = mem.alloc_short(7, 0);
                for i in 0..7 {
                    tup.set_val(i, Val::from_uint(0))
                }
                let val = Val(mem.stack[(acc % 4) as usize]);
                tup.set_val(0, val);
                mem.stack[(acc % 4) as usize] = tup.to_val().0;
            },
        }
    }
    println!("Minor.left = {}", mem.minor_pool.left);
    println!("Major.allocated = {}", mem.major_allocated);
}