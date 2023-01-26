use crate::gc::val::*;

use std::alloc::*;

pub fn alloc_bytes(bytes: usize) -> *mut u8 {
    let layout = Layout::from_size_align(bytes, WORD_SIZE).unwrap();
    unsafe {
        let ptr = alloc(layout);
        if ptr.is_null() {
            handle_alloc_error(layout);
        }
        ptr
    }
}

pub fn alloc_words(words: usize) -> *mut usize {
    let bytes = words.checked_mul(WORD_SIZE).expect("Allocation size overflow");
    alloc_bytes(bytes) as *mut usize
}

pub fn realloc_words(ptr: *mut usize, old_words: usize, new_words: usize) -> *mut usize {
    let old_bytes = old_words.checked_mul(WORD_SIZE).expect("Allocation size overflow");
    let new_bytes = new_words.checked_mul(WORD_SIZE).expect("Allocation size overflow");
    let layout = Layout::from_size_align(old_bytes, WORD_SIZE).unwrap();
    unsafe {
        let ptr = realloc(ptr as *mut u8, layout, new_bytes);
        if ptr.is_null() {
            handle_alloc_error(layout);
        }
        ptr as *mut usize
    }
}

pub fn dealloc_bytes(ptr: *mut u8, bytes: usize) {
    let layout = Layout::from_size_align(bytes, WORD_SIZE).unwrap();
    unsafe {
        dealloc(ptr, layout);
    }
}

pub fn dealloc_words(ptr: *mut usize, words: usize) {
    let bytes = words.checked_mul(WORD_SIZE).expect("Deallocation size overflow");
    let layout = Layout::from_size_align(bytes, WORD_SIZE).unwrap();
    unsafe {
        dealloc(ptr as *mut u8, layout);
    }
}

// Helper for major allocation
pub fn alloc_major_short(tup_list: &mut *mut usize, words: usize, tag: usize) -> Tup {
    let ptr = alloc_words(words + 1);
    unsafe {
        ptr.write(*tup_list as usize);
        *tup_list = ptr;
        ptr.add(1).write(Hd::short(words, tag).0);
        Tup(ptr.add(1))
    }
}

pub fn alloc_major_long(tup_list: &mut *mut usize, bytes: usize) -> Tup {
    let ptr = alloc_bytes(WORD_SIZE + bytes) as *mut usize;
    unsafe {
        ptr.write(*tup_list as usize);
        *tup_list = ptr;
        ptr.add(1).write(Hd::long(bytes).0);
        Tup(ptr.add(1))
    }
}

pub fn dealloc_major_next(tup_list: &mut *mut usize) {
    unsafe {
        let next = Tup(*tup_list);
        let next_next = next.next();
        *tup_list = next_next.0;
        if next.is_long() {
            dealloc_bytes((next.0.sub(1)) as *mut u8, next.header().long_bytes() + WORD_SIZE);
        } else {
            dealloc_words(next.0.sub(1), next.header().words() + 1);
        }
    }
}