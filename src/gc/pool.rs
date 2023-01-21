// pool: ssm memory pool

use std::alloc::{alloc, dealloc, handle_alloc_error, Layout};

use crate::gc::val::{Hd, Ptr, Tup, Uptr};

pub struct Pool {
    // Pool size informations
    pub bytes: usize,
    pub vals: Uptr,
    // Pool usage pointers
    pub left: Uptr,
    // Pool pointer
    pub ptr: Ptr,
}

impl Drop for Pool {
    fn drop(&mut self) {
        let word_size = std::mem::size_of::<Uptr>();
        let layout =
            Layout::from_size_align(self.bytes as usize, word_size).unwrap();
        unsafe {
            dealloc(self.ptr as *mut u8, layout);
        }
    }
}

impl Pool {
    pub fn new(bytes: usize) -> Pool {
        let word_size = std::mem::size_of::<Uptr>();
        let vals = (bytes / word_size) as Uptr;
        let ptr = unsafe {
            let layout = Layout::from_size_align(bytes, word_size).unwrap();
            let ptr = alloc(layout);
            if ptr.is_null() {
                handle_alloc_error(layout);
            }
            ptr as Ptr
        };
        Pool {
            bytes: bytes,
            vals: vals,
            left: vals,
            ptr: ptr,
        }
    }

    pub fn own(&self, ptr: Ptr) -> bool {
        let start = self.ptr as usize;
        let end = start + self.bytes;
        let ptr = ptr as usize;
        ptr >= start && ptr < end
    }

    pub fn rewind(&mut self) {
        self.left = self.vals;
    }

    pub fn tup(&self, offset: Uptr) -> Tup {
        unsafe { Tup(self.ptr.add(offset as usize)) }
    }

    pub fn alloc(&mut self, vals: Uptr) -> Result<Ptr, ()> {
        if self.left < vals {
            return Err(());
        }
        self.left -= vals;
        unsafe { Ok(self.ptr.add(self.left as usize)) }
    }

    #[inline(always)]
    pub fn allocatable_short(&self, vals: Uptr) -> bool {
        self.left >= Tup::short_size(vals)
    }

    #[inline(always)]
    pub fn allocatable_long(&self, bytes: Uptr) -> bool {
        self.left >= Tup::long_size(bytes)
    }

    pub fn alloc_short(&mut self, vals: Uptr, tag: Uptr) -> Result<Tup, ()> {
        let ptr = self.alloc(Tup::short_size(vals))?;
        let tup = Tup(ptr);
        tup.set_header(Hd::short(vals as Uptr, tag as Uptr));
        Ok(tup)
    }

    pub fn alloc_long(&mut self, bytes: Uptr) -> Result<Tup, ()> {
        let ptr = self.alloc(Tup::long_size(bytes))?;
        let tup = Tup(ptr);
        tup.set_header(Hd::long(bytes as Uptr));
        Ok(tup)
    }

    pub fn copy_tup(&mut self, tup: Tup) -> Result<Tup, ()> {
        let word_size = std::mem::size_of::<Uptr>();
        // Calculate tuple size
        let hd = tup.header();
        let size = if hd.is_long() {
            Tup::long_size(hd.long_size())
        } else {
            Tup::short_size(hd.size())
        };
        let new_tup = self.alloc(size)?;
        unsafe {
            std::ptr::copy_nonoverlapping(
                tup.0,
                new_tup,
                word_size * (size as usize),
            );
        }
        Ok(Tup(new_tup))
    }
}
