pub mod pool;
pub mod val;

use std::alloc::{alloc, dealloc, handle_alloc_error, realloc, Layout};

use crate::gc::pool::Pool;
use crate::gc::val::*;

pub struct Stack {
    pub size: Uptr,
    pub ptr: Ptr,
    pub top: Uptr,
}

impl Drop for Stack {
    fn drop(&mut self) {
        let word_size = std::mem::size_of::<Uptr>();
        let bytes = self.size as usize * word_size;
        let layout = Layout::from_size_align(bytes, word_size).unwrap();
        unsafe {
            dealloc(self.ptr as *mut u8, layout);
        }
    }
}

impl Stack {
    pub fn new(size: Uptr) -> Stack {
        let word_size = std::mem::size_of::<Uptr>();
        let bytes = size as usize * word_size;
        let ptr = unsafe {
            let layout = Layout::from_size_align_unchecked(bytes, word_size);
            let ptr = alloc(layout);
            if ptr.is_null() {
                handle_alloc_error(layout);
            }
            ptr as Ptr
        };
        Stack {
            size: size,
            ptr: ptr,
            top: 0,
        }
    }

    pub fn reserve(&mut self, vals: Uptr) {
        let new_top = self.top + vals;
        if new_top < self.size {
            return;
        }
        // Extend size
        let word_size = std::mem::size_of::<Uptr>();
        let new_size = std::cmp::max(self.size * 2, new_top + 1);
        let bytes = self.size as usize * word_size;
        let new_bytes = new_size as usize * word_size;
        let new_ptr = unsafe {
            let layout = Layout::from_size_align_unchecked(bytes, word_size);
            let new_ptr = realloc(self.ptr as *mut u8, layout, new_bytes);
            if new_ptr.is_null() {
                handle_alloc_error(layout);
            }
            new_ptr as Ptr
        };
        self.size = new_size;
        self.ptr = new_ptr;
    }

    pub fn get(&self, idx: Uptr) -> Val {
        Val(unsafe { *self.ptr.add(idx as usize) })
    }

    pub fn set(&self, idx: Uptr, val: Val) {
        unsafe {
            *self.ptr.add(idx as usize) = val.0;
        }
    }
}

pub struct Mem {
    pub global: Stack,
    pub stack: Stack,
    // pools[0]: Minor pool
    // pools[1..]: Major pools
    pub pools: [Pool; 2],
}

impl Mem {
    pub fn new(
        global_initial_vals: Uptr,
        stack_initial_vals: Uptr,
        minor_pool_size: usize,
        major_pool_size: usize,
    ) -> Mem {
        Mem {
            global: Stack::new(global_initial_vals),
            stack: Stack::new(stack_initial_vals),
            pools: [Pool::new(minor_pool_size), Pool::new(major_pool_size)],
        }
    }

    pub fn check_pointer_pool(&self, ptr: Ptr, limit: usize) -> bool {
        for i in 0..limit {
            if self.pools[i].own(ptr) {
                return true;
            }
        }
        false
    }

    pub fn mark_from_stack(&self, mark_limit: usize) -> Uptr {
        let mut marked_vals = 0;
        // mark_limit must be less than the number of pools
        if mark_limit > self.pools.len() {
            unreachable!();
        }
        // Create marked but not scanned list
        let mut marked = Vec::new();
        // Marking tuples keeping marked stack as small
        for i in 0..self.stack.top {
            // First, find from stack
            let val = self.stack.get(i);
            if val.is_gc_ptr() {
                let tup = Tup(val.to_gc_ptr());
                if self.check_pointer_pool(tup.0, mark_limit)
                    && tup.mark(Hd::COLOR_BLACK)
                {
                    marked_vals += tup.vals();
                    marked.push(tup);
                }
                while let Some(tup) = marked.pop() {
                    // Scan tuple
                    let hd = tup.header();
                    let vals = hd.size();
                    for i in 0..vals {
                        let val = tup.val(i);
                        if val.is_gc_ptr() {
                            let tup = Tup(val.to_gc_ptr());
                            if self.check_pointer_pool(tup.0, mark_limit)
                                && tup.mark(Hd::COLOR_BLACK)
                            {
                                marked_vals += tup.vals();
                                marked.push(tup);
                            }
                        }
                    }
                }
            }
        }
        marked_vals
    }

    pub fn move_pointers(
        src_pool: &Pool,
        dst_pool: &mut Pool,
    ) -> Result<(), ()> {
        let mut mp = src_pool.left;
        while mp < src_pool.vals {
            let tup = src_pool.tup(mp);
            let hd = tup.header();
            if hd.color() != Hd::COLOR_WHITE {
                // Allocate new memory in major pool
                let new_tup = dst_pool.copy_tup(tup)?;
                // Write new pointer to old memory
                unsafe {
                    tup.0.write(new_tup.0 as Uptr);
                }
            }
            mp += tup.vals();
        }
        Ok(())
    }

    pub fn update_pointers(
        &self,
        pool: &Pool,
        from: Uptr,
        to: Uptr,
        mark_limit: usize,
    ) {
        // Update pointers in tuples
        let mut off = from;
        while off < to {
            // Load tuple and move offset to next tuple
            let tup = pool.tup(off);
            off += tup.vals();
            // Unmark
            tup.unmark();
            // Pass if the tuple is not short one
            let hd = tup.header();
            if hd.is_long() {
                continue;
            }
            for i in 0..hd.size() {
                let val = tup.val(i);
                let ptr = val.to_gc_ptr();
                // If a gc pointer is in pool <= mark_limit, it may moved
                if val.is_gc_ptr() && self.check_pointer_pool(ptr, mark_limit) {
                    // Update pointer
                    let new_ptr = unsafe { (ptr as *mut Ptr).read() };
                    tup.set_val(i, Val::from_gc_ptr(new_ptr));
                }
            }
        }
    }

    pub fn collect_major(
        &mut self,
        required_free_vals: Uptr,
    ) -> Result<(), ()> {
        // Run marking process from stack
        let new_major_vals = self.mark_from_stack(1);
        // Calculate new major size
        let new_major_bytes = ((required_free_vals + new_major_vals) as usize)
            * std::mem::size_of::<Uptr>();
        let lb = std::cmp::max(self.pools[1].bytes / 2, self.pools[0].bytes);
        let new_major_bytes = std::cmp::max(new_major_bytes, lb);
        // Create new major pool
        let mut new_major_pool = Pool::new(new_major_bytes);
        // Copy marked tuples into new major pool
        for i in (0..self.pools.len()).rev() {
            Self::move_pointers(&self.pools[i], &mut new_major_pool)?;
        }
        // Update moved pointers and mark tuples white
        self.update_pointers(
            &new_major_pool,
            new_major_pool.left,
            new_major_pool.vals,
            1,
        );
        // Replace major pool
        self.pools[1] = new_major_pool;
        // Rewind minor pool
        self.pools[0].rewind();
        // Done
        Ok(())
    }

    pub fn collect_minor(&mut self) -> Result<(), ()> {
        // Calculate sizes
        let major_left = self.pools[1].left;
        let minor_allocated = self.pools[0].vals - self.pools[0].left;
        // Check major pool size
        if major_left < minor_allocated {
            // Just run major collection
            return self.collect_major(minor_allocated);
        }
        // Run marking process on minor pool
        self.mark_from_stack(0);
        // Move marked minor tuples into major pool
        let (minor, major) = self.pools.split_at_mut(1);
        Self::move_pointers(&minor[0], &mut major[0])?;
        // Update moved pointers and mark tuples white
        self.update_pointers(&self.pools[1], self.pools[1].left, major_left, 0);
        // Rewind minor pool
        self.pools[0].rewind();
        // Done
        Ok(())
    }

    pub fn alloc_short(&mut self, vals: Uptr, tag: Uptr) -> Result<Tup, ()> {
        if !self.pools[0].allocatable_short(vals) {
            self.collect_minor()?;
        }
        self.pools[0].alloc_short(vals, tag)
    }

    pub fn alloc_long(&mut self, bytes: Uptr) -> Result<Tup, ()> {
        let size = Tup::long_size(bytes);
        if self.pools[0].vals < size {
            // Minor is too small, allocate in major
            if !self.pools[1].allocatable_long(bytes) {
                self.collect_major(bytes)?;
            }
            self.pools[1].alloc_long(bytes)
        } else {
            if !self.pools[0].allocatable_long(bytes) {
                self.collect_minor()?;
            }
            self.pools[0].alloc_long(bytes)
        }
    }

    pub fn reserve_minor(&mut self, vals: Uptr) -> Result<(), ()> {
        if self.pools[0].left < vals {
            self.collect_minor()?;
        }
        Ok(())
    }
}
