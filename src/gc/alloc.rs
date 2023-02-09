use crate::gc::val::*;

use std::alloc::*;

#[inline(always)]
unsafe fn words_layout(words: usize) -> Layout {
    let bytes = words
        .checked_mul(WORD_SIZE)
        .expect("Allocation size overflow");
    Layout::from_size_align_unchecked(bytes, WORD_SIZE)
}

#[inline(always)]
pub unsafe fn alloc_words(words: usize) -> *mut usize {
    let ptr = alloc(words_layout(words));
    if ptr.is_null() {
        handle_alloc_error(words_layout(words));
    }
    ptr as *mut usize
}

#[inline(always)]
pub unsafe fn alloc_bytes(bytes: usize) -> *mut u8 {
    let words = bytes
        .checked_add(WORD_SIZE - 1)
        .expect("Allocation size overflow")
        / WORD_SIZE;
    alloc_words(words) as *mut u8
}

pub unsafe fn realloc_words(
    ptr: *mut usize,
    old_words: usize,
    new_words: usize,
) -> *mut usize {
    let new_bytes = new_words
        .checked_mul(WORD_SIZE)
        .expect("Allocation size overflow");
    let ptr = realloc(ptr as *mut u8, words_layout(old_words), new_bytes);
    if ptr.is_null() {
        handle_alloc_error(words_layout(old_words));
    }
    ptr as *mut usize
}

#[inline(always)]
pub unsafe fn dealloc_words(ptr: *mut usize, words: usize) {
    dealloc(ptr as *mut u8, words_layout(words));
}

#[inline(always)]
pub unsafe fn dealloc_bytes(ptr: *mut u8, bytes: usize) {
    let words = bytes
        .checked_add(WORD_SIZE - 1)
        .expect("Allocation size overflow")
        / WORD_SIZE;
    dealloc_words(ptr as *mut usize, words);
}

// Helper for major allocation
pub unsafe fn alloc_major_short(
    tup_list: &mut *mut usize,
    words: usize,
    tag: usize,
) -> Tup {
    let ptr = alloc_words(1 + Tup::words_from_words(words));
    ptr.write((*tup_list) as usize);
    let ptr = ptr.add(1);
    *tup_list = ptr;
    ptr.write(Hd::short(words, tag).0);
    Tup(ptr)
}

pub unsafe fn alloc_major_long(tup_list: &mut *mut usize, bytes: usize) -> Tup {
    let ptr = alloc_words(1 + Tup::words_from_bytes(bytes)) as *mut usize;
    ptr.write(*tup_list as usize);
    let ptr = ptr.add(1);
    *tup_list = ptr;
    ptr.write(Hd::long(bytes).0);
    Tup(ptr)
}

pub unsafe fn dealloc_major_next(tup_list: *mut *mut usize) -> usize {
    let next = Tup(*tup_list);
    let next_next = next.next();
    tup_list.write(next_next.0);
    let words = next.header().words();
    dealloc_words(next.0.sub(1), words + 1);
    words
}
