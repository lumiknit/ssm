// pool: ssm memory pool for minor heap

use crate::gc::alloc::*;
use crate::gc::val::*;

pub struct Pool {
    // Pool size informations
    pub bytes: usize,
    pub words: usize,
    // Pool usage pointers
    pub left: usize,
    // Pool pointer
    pub ptr: *mut usize,
}

impl Drop for Pool {
    fn drop(&mut self) {
        unsafe { dealloc_words(self.ptr, self.words) }
    }
}

impl Pool {
    pub fn new(bytes: usize) -> Self {
        let words = (bytes / WORD_SIZE) as usize;
        let ptr = unsafe { alloc_words(words) };
        Self {
            bytes: bytes,
            words,
            left: words,
            ptr: ptr,
        }
    }

    #[inline(always)]
    pub fn own(&self, ptr: *mut usize) -> bool {
        let start = self.ptr as usize;
        let end = start + self.bytes;
        let ptr = ptr as usize;
        ptr >= start && ptr < end
    }

    #[inline(always)]
    pub fn own_tup(&self, tup: Tup) -> bool {
        self.own(tup.0)
    }

    #[inline(always)]
    pub fn rewind(&mut self) {
        self.left = self.words;
    }

    #[inline(always)]
    pub fn tup(&self, offset: usize) -> Tup {
        unsafe { Tup(self.ptr.add(offset as usize)) }
    }

    pub fn alloc_unchecked(&mut self, words: usize) -> *mut usize {
        self.left -= words;
        unsafe { self.ptr.add(self.left as usize) }
    }

    pub fn alloc(&mut self, words: usize) -> Option<*mut usize> {
        if self.left < words {
            return None;
        }
        Some(self.alloc_unchecked(words))
    }

    #[inline(always)]
    pub fn allocatable_in_words(&self, words: usize) -> bool {
        self.left >= Tup::words_from_words(words)
    }

    #[inline(always)]
    pub fn allocatable_in_bytes(&self, bytes: usize) -> bool {
        self.left >= Tup::words_from_bytes(bytes)
    }

    pub fn alloc_short(&mut self, words: usize, tag: usize) -> Option<Tup> {
        let ptr = self.alloc(Tup::words_from_words(words))?;
        let tup = Tup(ptr);
        tup.set_header(Hd::short(words as usize, tag as usize));
        Some(tup)
    }

    pub fn alloc_short_unchecked(&mut self, words: usize, tag: usize) -> Tup {
        let ptr = self.alloc_unchecked(Tup::words_from_words(words));
        let tup = Tup(ptr);
        tup.set_header(Hd::short(words as usize, tag as usize));
        tup
    }

    pub fn alloc_long(&mut self, bytes: usize) -> Option<Tup> {
        let ptr = self.alloc(Tup::words_from_bytes(bytes))?;
        let tup = Tup(ptr);
        tup.set_header(Hd::long(bytes as usize));
        Some(tup)
    }

    pub fn alloc_long_unchecked(&mut self, bytes: usize) -> Tup {
        let ptr = self.alloc_unchecked(Tup::words_from_bytes(bytes));
        let tup = Tup(ptr);
        tup.set_header(Hd::long(bytes as usize));
        tup
    }
}
