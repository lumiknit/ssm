pub mod alloc;
pub mod pool;
pub mod val;

use std::ptr;

use crate::gc::alloc::*;
use crate::gc::pool::Pool;
use crate::gc::val::*;

pub struct Mem {
    // Global stack
    pub global: Vec<*mut usize>,
    // Call stack
    pub stack: Vec<*mut usize>,
    // Minor memory pool
    pub minor_pool: Pool,
    // Major object linked list
    // Objects which cannot be dropped until the mem is dropped
    pub major_immortal: *mut usize,
    // Objects which does not contain any other pointers managed by GC
    pub major_leaves: *mut usize,
    // Other objects
    pub major_shorts: *mut usize,
}

struct MarkState {
    limit: usize,
    marked_vals: usize,
    marked: Vec<Tup>,
}

impl Mem {
    pub fn new(
        global_initial_vals: usize,
        stack_initial_vals: usize,
        minor_pool_size: usize,
    ) -> Mem {
        Mem {
            global: Vec::with_capacity(global_initial_vals),
            stack: Vec::with_capacity(stack_initial_vals),
            minor_pool: Pool::new(minor_pool_size),
            major_immortal: ptr::null_mut(),
            major_leaves: ptr::null_mut(),
            major_shorts: ptr::null_mut(),
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

    #[inline(always)]
    fn mark_val(&self, val: Val, state: &mut MarkState) {
        if val.is_gc_ptr() {
            let tup = Tup::from_val(val);
            if self.check_pointer_pool(tup.0, state.limit)
                && tup.mark(Hd::COLOR_BLACK)
            {
                state.marked_vals += tup.vals();
                state.marked.push(tup);
            }
            while let Some(tup) = state.marked.pop() {
                // Scan tuple
                let hd = tup.header();
                let vals = hd.size();
                for i in 0..vals {
                    let val = tup.val(i);
                    if val.is_gc_ptr() {
                        let tup = Tup(val.to_gc_ptr());
                        if self.check_pointer_pool(tup.0, state.limit)
                            && tup.mark(Hd::COLOR_BLACK)
                        {
                            state.marked_vals += tup.vals();
                            state.marked.push(tup);
                        }
                    }
                }
            }
        }
    }

    pub fn mark_from_stack(&self, mark_limit: usize) -> usize {
        // mark_limit must be less than the number of pools
        if mark_limit > self.pools.len() {
            unreachable!();
        }
        let mut state = MarkState {
            limit: mark_limit,
            marked_vals: 0,
            marked: Vec::new(),
        };
        // Marking tuples keeping marked stack as small
        for i in 0..self.global.top {
            self.mark_val(self.global.get(i), &mut state);
        }
        for i in 0..self.stack.top {
            self.mark_val(self.stack.get(i), &mut state);
        }
        state.marked_vals
    }

    pub fn move_pointers(
        src_pool: &Pool,
        dst_pool: &mut Pool,
    ) -> Result<(), ()> {
        let mut mp = src_pool.left;
        while mp < src_pool.vals {
            let tup = src_pool.tup(mp);
            let hd = tup.header();
            mp += tup.vals();
            if hd.color() != Hd::COLOR_WHITE {
                // Allocate new memory in major pool
                let new_tup = dst_pool.copy_tup(tup)?;
                // Write new pointer to old memory
                unsafe {
                    tup.0.write(new_tup.0 as usize);
                }
            }
        }
        Ok(())
    }

    pub fn update_pointers(
        &self,
        pool: &Pool,
        from: usize,
        to: usize,
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

    pub fn update_stack_pointers(&self, stack: &Stack, mark_limit: usize) {
        for i in 0..stack.top {
            let val = stack.get(i);
            let ptr = val.to_gc_ptr();
            // If a gc pointer is in pool <= mark_limit, it may moved
            if val.is_gc_ptr() && self.check_pointer_pool(ptr, mark_limit) {
                // Update pointer
                let new_ptr = unsafe { (ptr as *mut Ptr).read() };
                stack.set(i, Val::from_gc_ptr(new_ptr));
            }
        }
    }

    pub fn collect_major(
        &mut self,
        required_free_vals: usize,
    ) -> Result<(), ()> {
        let mark_limit = self.pools.len();
        // Run marking process from stack
        let new_major_vals = self.mark_from_stack(mark_limit);
        // Calculate new major size
        let new_major_bytes = ((required_free_vals + new_major_vals) as usize)
            * 2
            * std::mem::size_of::<usize>();
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
            mark_limit,
        );
        self.update_stack_pointers(&self.global, mark_limit);
        self.update_stack_pointers(&self.stack, mark_limit);
        // Replace major pool
        self.pools[1] = new_major_pool;
        // Rewind minor pool
        self.pools[0].rewind();
        // Done
        Ok(())
    }

    pub fn collect_minor(&mut self) -> Result<(), ()> {
        let mark_limit = 1;
        // Calculate sizes
        let major_left = self.pools[1].left;
        let minor_allocated = self.pools[0].vals - self.pools[0].left;
        // Check major pool size
        if major_left < minor_allocated {
            // Just run major collection
            return self.collect_major(minor_allocated);
        }
        // Run marking process on minor pool
        self.mark_from_stack(mark_limit);
        // Move marked minor tuples into major pool
        let (minor, major) = self.pools.split_at_mut(1);
        Self::move_pointers(&minor[0], &mut major[0])?;
        // Update moved pointers and mark tuples white
        self.update_pointers(
            &self.pools[1],
            self.pools[1].left,
            major_left,
            mark_limit,
        );
        self.update_stack_pointers(&self.global, mark_limit);
        self.update_stack_pointers(&self.stack, mark_limit);
        // Rewind minor pool
        self.pools[0].rewind();
        // Done
        Ok(())
    }

    pub fn alloc_short(&mut self, vals: usize, tag: usize) -> Result<Tup, ()> {
        if !self.pools[0].allocatable_short(vals) {
            self.collect_minor()?;
        }
        self.pools[0].alloc_short(vals, tag)
    }

    pub fn alloc_long(&mut self, bytes: usize) -> Result<Tup, ()> {
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

    pub fn reserve_minor(&mut self, vals: usize) -> Result<(), ()> {
        if self.pools[0].left < vals {
            self.collect_minor()?;
        }
        Ok(())
    }
}
