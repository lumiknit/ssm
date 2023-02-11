pub mod alloc;
pub mod pool;
pub mod val;

use std::ptr;

use crate::gc::alloc::*;
use crate::gc::pool::Pool;
use crate::gc::val::*;

pub struct Mem {
    // Global stack
    pub global: Vec<usize>,
    // Call stack
    pub stack: Vec<usize>,

    // Minor memory pool
    pub minor_pool: Pool,

    // Major object linked list
    // Objects which cannot be dropped until the mem is dropped
    pub major_immortal: *mut usize,
    // Objects which does not contain any other pointers managed by GC
    pub major_leaves: *mut usize,
    // Other objects
    pub major_nodes: *mut usize,

    pub major_gc_thresold_percent: usize,
    pub major_allocated: usize,
    pub major_limit: usize,
}

struct MarkState {
    marked_words: usize,
    marked: Vec<Tup>,
}

impl Drop for Mem {
    fn drop(&mut self) {
        // Drop lists
        let im: *mut *mut usize = &mut self.major_immortal;
        let le: *mut *mut usize = &mut self.major_leaves;
        let sh: *mut *mut usize = &mut self.major_nodes;
        for lst in [im, le, sh].into_iter() {
            unsafe {
                while !(*lst).is_null() {
                    self.major_allocated -= dealloc_major_next(lst);
                }
            }
        }
    }
}

impl Mem {
    pub fn new(
        global_initial_vals: usize,
        stack_initial_vals: usize,
        mut minor_pool_bytes: usize,
        mut major_gc_threshold_percent: usize,
    ) -> Mem {
        // Clip minor_pool_size
        if minor_pool_bytes >= usize::MAX / 8 {
            minor_pool_bytes = usize::MAX / 8;
        }
        if major_gc_threshold_percent >= 256 {
            major_gc_threshold_percent = 255;
        }
        let mut this = Mem {
            global: Vec::with_capacity(global_initial_vals),
            stack: Vec::with_capacity(stack_initial_vals),
            minor_pool: Pool::new(minor_pool_bytes),
            major_immortal: ptr::null_mut(),
            major_leaves: ptr::null_mut(),
            major_nodes: ptr::null_mut(),
            major_gc_thresold_percent: major_gc_threshold_percent,
            major_allocated: 0,
            major_limit: 0,
        };
        this.update_major_limit();
        this
    }

    pub fn update_major_limit(&mut self) {
        // If major_gc_threshold_percent is 0, disable major GC
        if self.major_gc_thresold_percent == 0 {
            self.major_limit = usize::MAX;
            return;
        }
        // Calculate minimal pool size
        // = min_pool_size * 7
        let min_size = self.minor_pool.words.checked_mul(7)
            .unwrap_or(usize::MAX);
        // Calculate limit with percent-value
        // new_limit = allocated * (1 + 0.01 * threshold_percent)
        // e.g. threshold_percent = 150, limit ratio = 2.5
        // Split allocated into two part to avoid overflow
        let lo: usize = (self.major_allocated % 100)
            * self.major_gc_thresold_percent
            / 100;
        let hi: usize = (self.major_allocated / 100)
            .checked_mul(self.major_gc_thresold_percent)
            .unwrap_or(usize::MAX);
        // Add them
        let additional_words = hi.checked_add(lo).unwrap_or(usize::MAX);
        let new_limit = self.major_allocated
            .checked_add(additional_words)
            .unwrap_or(usize::MAX);
        self.major_limit = usize::max(min_size, new_limit);
    }

    fn mark_val<F>(val: Val, state: &mut MarkState, markable_ptr: F)
    where
        F: Fn(*mut usize) -> bool,
    {
        if !val.is_gc_ptr() { return; }
        let tup = Tup::from_val(val);
        if !markable_ptr(tup.0) || !tup.mark(Hd::COLOR_BLACK) { return; }
        state.marked_words += tup.words();
        if tup.is_long() { return; }
        state.marked.push(tup);
        while let Some(tup) = state.marked.pop() {
            for i in 0..tup.header().short_words() {
                let val = tup.val(i);
                if !val.is_gc_ptr() { continue; }
                let tup = Tup::from_val(val);
                if !markable_ptr(tup.0) || !tup.mark(Hd::COLOR_BLACK) { continue; }
                state.marked_words += tup.words();
                if tup.is_long() { continue; }
                state.marked.push(tup);
            }
        }
    }

    pub fn mark_major(&mut self) -> usize {
        let mut state = MarkState {
            marked_words: 0,
            marked: Vec::new(),
        };
        let markable_ptr = |ptr: *mut usize| !ptr.is_null();
        // Mark global stack
        for val in self.global.iter() {
            Self::mark_val(Val(*val), &mut state, &markable_ptr);
        }
        // Mark call stack
        for val in self.stack.iter() {
            Self::mark_val(Val(*val), &mut state, &markable_ptr);
        }
        // Return marked words
        state.marked_words
    }

    pub fn mark_minor(&mut self) -> usize {
        let mut state = MarkState {
            marked_words: 0,
            marked: Vec::new(),
        };
        let markable_ptr = |ptr: *mut usize| self.minor_pool.own(ptr);
        // Mark global stack
        for val in self.global.iter() {
            Self::mark_val(Val(*val), &mut state, &markable_ptr);
        }
        // Mark call stack
        for val in self.stack.iter() {
            Self::mark_val(Val(*val), &mut state, &markable_ptr);
        }
        // Return marked words
        state.marked_words
    }

    #[inline(always)]
    pub fn get_new_address(minor_pool: &Pool, val: Val) -> Option<Val> {
        if val.is_gc_ptr() && minor_pool.own(val.to_gc_ptr()) {
            unsafe {
                Some(Val::from_gc_ptr(
                    val.to_gc_ptr::<usize>().read() as *mut usize
                ))
            }
        } else {
            None
        }
    }

    pub fn move_minor_to_major(&mut self) {
        // First, record last short list
        let old_major_nodes = self.major_nodes;
        // First, copy all marked objects to major heap
        // and write new address into old tuple
        unsafe {
            let mut ptr = self.minor_pool.ptr.add(self.minor_pool.left);
            let lim = self.minor_pool.ptr.add(self.minor_pool.words);
            while ptr < lim {
                let tup = Tup(ptr);
                let hd = tup.header();
                let words = hd.words();
                if !hd.is_white() {
                    let new_tup = if hd.is_long() {
                        self.alloc_major_long(hd.long_bytes())
                    } else {
                        self.alloc_major_short(words, hd.tag())
                    };
                    ptr::copy_nonoverlapping::<usize>(
                        ptr.add(1),
                        new_tup.0.add(1),
                        words,
                    );
                    tup.0.write(new_tup.0 as usize);
                }
                ptr = ptr.add(Tup::words_from_words(words));
            }
        }
        // Traverse all short list and re-addressing
        let mut tup = Tup(self.major_nodes);
        while !tup.0.is_null() && tup.0 != old_major_nodes {
            let hd = tup.header();
            let words = hd.short_words();
            for i in 0..words {
                if let Some(new) =
                    Self::get_new_address(&self.minor_pool, tup.val(i))
                {
                    tup.set_val(i, new);
                }
            }
            tup = tup.next();
        }
        // Traverse all global & stack and re-addressing
        for val in self.global.iter_mut() {
            if let Some(new) =
                Self::get_new_address(&self.minor_pool, Val(*val))
            {
                *val = new.0;
            }
        }
        for val in self.stack.iter_mut() {
            if let Some(new) =
                Self::get_new_address(&self.minor_pool, Val(*val))
            {
                *val = new.0;
            }
        }
    }

    pub fn collect_major(&mut self) {
        // Run marking phase
        let _marked_words = self.mark_major();
        // Traverse object list and free unmarked objects,
        // also unmark marked objects
        let leaves: *mut *mut usize = &mut self.major_leaves;
        let shorts: *mut *mut usize = &mut self.major_nodes;
        for mut lst in [leaves, shorts].into_iter() {
            loop {
                unsafe {
                    let next = *lst;
                    let next_tup = Tup(next);
                    if next.is_null() {
                        break;
                    } else if next_tup.header().is_white() {
                        self.major_allocated -= dealloc_major_next(lst);
                    } else {
                        next_tup.unmark();
                        lst = next_tup.next_ptr::<*mut usize>();
                    }
                }
            }
        }
        // Move marked object in minor heap to major heap
        self.move_minor_to_major();
        // Rewind pointer
        self.minor_pool.rewind();
        // Adjust next limit
        self.update_major_limit();
    }

    pub fn collect_minor(&mut self) {
        // If major heap is full, run major collect
        if self.major_allocated >= self.major_limit {
            return self.collect_major();
        }
        // Run marking phase
        let _marked_words = self.mark_minor();
        // Move marked objects to major heap
        self.move_minor_to_major();
        // Rewind pointer
        self.minor_pool.rewind();
    }

    pub fn alloc_major_long(&mut self, bytes: usize) -> Tup {
        let tup_words = Tup::words_from_bytes(bytes);
        self.major_allocated = self.major_allocated.saturating_add(tup_words);
        unsafe { alloc_major_long(&mut self.major_leaves, bytes) }
    }

    pub fn alloc_major_short(&mut self, words: usize, tag: usize) -> Tup {
        let tup_words = Tup::words_from_words(words);
        self.major_allocated = self.major_allocated.saturating_add(tup_words);
        unsafe { alloc_major_short(&mut self.major_nodes, words, tag) }
    }

    pub fn alloc_short(&mut self, words: usize, tag: usize) -> Tup {
        let tup_words = Tup::words_from_words(words);
        // If minor pool is too small to contain the tuple,
        // try to allocate in major heap
        if self.minor_pool.words < tup_words {
            // If some objects exist in minor heap, try to collect them
            // for object age invariant
            if self.minor_pool.left < self.minor_pool.words {
                self.collect_minor();
            }
            // Allocate object in major heap
            self.alloc_major_short(words, tag)
        } else {
            // Try to allocate in minor heap
            if self.minor_pool.left < tup_words {
                self.collect_minor();
            }
            self.minor_pool.alloc_short_unchecked(words, tag)
        }
    }

    pub fn alloc_long(&mut self, bytes: usize) -> Tup {
        let tup_words = Tup::words_from_bytes(bytes);
        // If minor pool is too small to contain the tuple,
        // try to allocate in major heap
        if self.minor_pool.words < tup_words {
            self.alloc_major_long(bytes)
        } else {
            // Try to allocate in minor heap
            if self.minor_pool.left < tup_words {
                self.collect_minor();
            }
            self.minor_pool.alloc_long_unchecked(bytes)
        }
    }

    pub fn reserve_minor(&mut self, vals: usize) {
        if self.minor_pool.left < vals {
            self.collect_minor()
        }
    }
}
