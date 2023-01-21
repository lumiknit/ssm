// mem.rs: memory allocator & collector

use std::alloc::{alloc, dealloc, handle_alloc_error, Layout};

use crate::gc::val::{Uptr, Hd, Tup};
use crate::gc::pool::{Pool};

pub struct Stack {
    pub size: usize,
    pub ptr: *mut Uptr,
}

impl Stack {
    pub fn new(size: usize) -> Stack {
        let word_size = std::mem::size_of::<Uptr>();
        let ptr = unsafe {
            let layout = Layout::from_size_align(word_size * size, word_size).unwrap();
            let ptr = alloc(layout);
            if ptr.is_null() {
                handle_alloc_error(layout);
            }
            ptr as *mut Uptr
        };
        Stack {
            size: size,
            ptr: ptr,
        }
    }

    pub fn reserve(&mut self, more_size: usize) {
        let new_size = self.size + more_size;
        if new_size > self.size {
            let word_size = std::mem::size_of::<usize>();
            let layout = Layout::from_size_align(self.size, word_size).unwrap();
            unsafe {
                dealloc(self.ptr as *mut u8, layout);
            }
            let ptr = unsafe {
                let layout = Layout::from_size_align(new_size, word_size).unwrap();
                let ptr = alloc(layout);
                if ptr.is_null() {
                    handle_alloc_error(layout);
                }
                ptr as *mut Uptr
            };
            self.size = new_size;
            self.ptr = ptr;
        }
    }
}

pub struct Mem {
    pub stack: Stack,
    pub minor_pool: Pool,
    pub major_pool: Pool,
}

impl Mem {
    pub fn new(stack_size: usize, minor_pool_size: usize, major_pool_size: usize) -> Mem {
        Mem {
            stack: Stack::new(stack_size),
            minor_pool: Pool::new(minor_pool_size),
            major_pool: Pool::new(major_pool_size),
        }
    }

    pub fn collect_major(&mut self, required_free_size: usize) -> Result<(), ()> {
        // Run marking process from stack
        let mut new_major_size = 0;
        unimplemented!();
        // Calculate new major size
        new_major_size += required_free_size;
        let lb = std::cmp::max(self.major_pool.vals / 2, self.minor_pool.vals);
        new_major_size = std::cmp::max(new_major_size, lb);
        // Create new major pool
        let mut new_major_pool = Pool::new(new_major_size);
        // Copy marked tuples into new major pool
        unimplemented!();
        // Rewrite pointers in moved tuples
        unimplemented!();
     }

    pub fn collect_minor(&mut self) -> Result<(), ()> {
        // Calculate sizes
        let major_left = self.major_pool.left;
        let minor_allocated = self.minor_pool.vals - self.minor_pool.left;
        // Check major pool size
        if major_left < minor_allocated {
            // Try to collect major
            self.collect_major(minor_allocated)?;
        }
        // Run marking process on minor pool
        unimplemented!();
        // Copy marked minor tuples into major pool
        unimplemented!();
        // Rewrite pointers in moved major tuples
        unimplemented!();
    }
    
    pub fn alloc_short(&mut self, vals: usize, tag: Uptr) -> Result<Tup, ()> {
        if !self.minor_pool.allocatable_short(vals) {
            self.collect_minor()?;
        }
        self.minor_pool.alloc_short(vals, tag)
    }

    pub fn alloc_long(&mut self, bytes: usize) -> Result<Tup, ()> {
        let size = Tup::long_size(bytes);
        if self.minor_pool.vals < size {
            // Minor is too small, allocate in major
            if !self.major_pool.allocatable_long(bytes) {
                self.collect_major(bytes)?;
            }
            self.major_pool.alloc_long(bytes)
        } else {
            if !self.minor_pool.allocatable_long(bytes) {
                self.collect_minor()?;
            }
            self.minor_pool.alloc_long(bytes)
        }
    }

    pub fn reserve_minor(&mut self, vals: usize) -> Result<(), ()> {
        if self.minor_pool.left < vals {
            self.collect_minor()?;
        }
        Ok(())
    }
}