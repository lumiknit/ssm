pub mod alloc;
pub mod val;
pub mod pool;

use crate::gc::val::*;
use crate::gc::alloc::*;
use crate::gc::pool::Pool;

pub struct Stack {
    pub size: usize,
    pub ptr: *mut usize,
    pub top: usize,
}

impl Drop for Stack {
    fn drop(&mut self) {
        let word_size = std::mem::size_of::<usize>();
        let bytes = self.size as usize * word_size;
        let layout = Layout::from_size_align(bytes, word_size).unwrap();
        unsafe {
            dealloc(self.ptr as *mut u8, layout);
        }
    }
}

impl Stack {
    pub fn new(size: usize) -> Stack {
        let word_size = std::mem::size_of::<usize>();
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

    pub fn reserve(&mut self, vals: usize) -> usize {
        let old_top = self.top;
        let new_top = old_top + vals;
        if new_top < self.size {
            return old_top;
        }
        // Extend size
        let word_size = std::mem::size_of::<usize>();
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
        old_top
    }

    #[inline(always)]
    pub fn move_top_unchecked(&mut self, vals: isize) {
        self.top += vals as usize;
    }

    pub fn move_top(&mut self, vals: isize) {
        let mut new_top = (self.top as isize) + vals;
        if new_top < 0 {
            new_top = 0;
        }
        let new_top = new_top as usize;
        if new_top > self.size {
            self.reserve(new_top - self.size);
        }
        self.top = new_top;
    }

    #[inline(always)]
    pub fn push_unchecked(&mut self, val: Val) {
        unsafe {
            *self.ptr.add(self.top as usize) = val.0;
        }
        self.top += 1;
    }

    pub fn push(&mut self, val: Val) {
        if self.top >= self.size {
            self.reserve(self.top + 1 - self.size);
        }
        self.push_unchecked(val)
    }

    #[inline(always)]
    pub fn pop_unchecked(&mut self) -> Val {
        self.top -= 1;
        Val(unsafe { *self.ptr.add(self.top as usize) })
    }

    pub fn pop(&mut self) -> Option<Val> {
        if self.top == 0 {
            None
        } else {
            Some(self.pop_unchecked())
        }
    }

    pub fn get(&self, idx: usize) -> Val {
        Val(unsafe { *self.ptr.add(idx as usize) })
    }

    pub fn set(&self, idx: usize, val: Val) {
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
