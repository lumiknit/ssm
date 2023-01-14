// pool: ssm memory pool

use std::alloc::{alloc, dealloc, handle_alloc_error, Layout};

use crate::gc::val::{Uptr, Hd, Tup};

pub struct Pool {
    // Pool size informations
    pub bytes: usize,
    pub vals: usize,
    // Pool usage pointers
    pub left: usize,
    // Pool pointer
    pub ptr: *mut Uptr,
}

impl Drop for Pool {
    fn drop(&mut self) {
        let word_size = std::mem::size_of::<usize>();
        let layout = Layout::from_size_align(self.bytes, word_size).unwrap();
        unsafe {
            dealloc(self.ptr as *mut u8, layout);
        }
    }
}

impl Pool {
    pub fn new(bytes: usize) -> Pool {
        let word_size = std::mem::size_of::<usize>();
        let vals = bytes / word_size;
        let ptr = unsafe {
            let layout = Layout::from_size_align(bytes, word_size).unwrap();
            let ptr = alloc(layout);
            if ptr.is_null() {
                handle_alloc_error(layout);
            }
            ptr as *mut Uptr
        };
        Pool {
            bytes: bytes,
            vals: vals,
            left: vals,
            ptr: ptr,
        }
    }
    
    pub fn rewind(&mut self) {
        self.left = self.vals;
    }

    pub fn alloc(&mut self, vals: usize) -> Result<*mut Uptr, ()> {
        if self.left < vals {
            return Err(());
        }
        self.left -= vals;
        unsafe {
            Ok(self.ptr.add(self.left))
        }
    }

    pub fn alloc_bytes(&mut self, bytes: usize) -> Result<*mut Uptr, ()> {
        let word_size = std::mem::size_of::<Uptr>();
        let vals = (bytes + word_size - 1) / word_size;
        self.alloc(vals)
    }

    pub fn alloc_short(&mut self, vals: usize, tag: Uptr) -> Result<Tup, ()> {
        let ptr = self.alloc(vals + 1)?;
        let tup = Tup(ptr);
        tup.set_header(Hd::short(vals as Uptr, tag as Uptr));
        Ok(tup)
    }

    pub fn alloc_long(&mut self, bytes: usize) -> Result<Tup, ()> {
        let word_size = std::mem::size_of::<Uptr>();
        let ptr = self.alloc_bytes(word_size + bytes)?;
        let tup = Tup(ptr);
        tup.set_header(Hd::long(bytes as Uptr));
        Ok(tup)
    }
}
