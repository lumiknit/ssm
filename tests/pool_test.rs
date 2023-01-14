use ssm::gc::pool::Pool;

#[test]
fn always_pass() {
    assert_eq!(1, 1);
}


#[test]
fn create_pool() {
    let pool = Pool::new(1024);
}

#[test]
fn allocate_values() {
    let mut pool = Pool::new(1024);
    let old_left = pool.left;
    // Allocate
    let tup = pool.alloc(16);
    assert!(tup.is_ok());
    assert_eq!(old_left, pool.left + 16);
    // Check pointer
    let tup = tup.unwrap();
    assert_eq!(pool.left, old_left - 16);
    unsafe { assert_eq!(tup, pool.ptr.add(old_left - 16)); }
}

#[test]
fn allocate_two_values() {
    let mut pool = Pool::new(1024);
    // Allocate one
    let tup1 = pool.alloc(16);
    assert!(tup1.is_ok());
    // Allocate another one
    let tup2 = pool.alloc(16);
    assert!(tup2.is_ok());
    // Check pointer
    unsafe {
        tup2.unwrap().add(16).write(42);
        assert_eq!(tup1.unwrap().read(), 42);
    }
}

#[test]
fn allocate_bytes() {
    const WORD_SIZE: usize = std::mem::size_of::<usize>();
    let mut pool = Pool::new(1024);
    // Allocate 1
    let saved_left_1 = pool.left;
    let bytes1 = pool.alloc_bytes(17);
    assert_eq!(pool.left, saved_left_1 - (17 / WORD_SIZE) - 1);
    // Allocate 2
    let saved_left_2 = pool.left;
    let _bytes2 = pool.alloc_bytes(35);
    assert_eq!(pool.left, saved_left_2 - (35 / WORD_SIZE) - 1);
    // Allooate last
    let saved_left_3 = pool.left;
    let bytes3 = pool.alloc_bytes(48);
    assert_eq!(pool.left, saved_left_3 - (48 / WORD_SIZE));
    // ptrs
    let p1 = bytes1.unwrap();
    let p3 = bytes3.unwrap();
    unsafe {
        p3.write_bytes(0, 100);
        assert_eq!((p1 as *const u8).read(), 0);
        p3.write_bytes(31, 100);
        assert_eq!((p1 as *const u8).read(), 31);
    }
}

#[test]
fn allocation_failed() {
    const WORD_SIZE: usize = std::mem::size_of::<usize>();
    let mut pool = Pool::new(WORD_SIZE * 100);
    assert!(pool.alloc(30).is_ok());
    assert!(pool.alloc(100).is_err());
    assert!(pool.alloc(71).is_err());
    assert!(pool.alloc_bytes(1 + WORD_SIZE * 70).is_err());
    assert!(pool.alloc_bytes(12345).is_err());
    assert!(pool.alloc_bytes(WORD_SIZE * 70).is_ok());
    assert!(pool.alloc_bytes(WORD_SIZE * 70).is_err());
}

#[test]
fn allocate_short() {
    const WORD_SIZE: usize = std::mem::size_of::<usize>();
    let mut pool = Pool::new(1024);
    let tup = pool.alloc_short(4, 12);
    assert!(tup.is_ok());
    let tup = tup.unwrap();
    let hd = tup.header();
    assert!(!hd.is_long());
    assert!(hd.is_black());
    assert_eq!(hd.tag(), 12);
    assert_eq!(hd.size(), 4);
    assert_eq!(pool.left, (1024 / WORD_SIZE) - 5);
    unsafe {
        assert_eq!(tup.0, pool.ptr.add(pool.left));
        assert_eq!(tup.0.read(), hd.0);
    }
}