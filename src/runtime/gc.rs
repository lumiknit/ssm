use std::alloc::*;

use crate::core::val::*;

/* Alloc
 */

#[inline(always)]
unsafe fn words_layout(words: usize) -> Layout {
  let bytes = words * WORD_SIZE;
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
  let words = (bytes + WORD_SIZE - 1) / WORD_SIZE;
  alloc_words(words) as *mut u8
}

pub unsafe fn realloc_words(
    ptr: *mut usize,
    old_words: usize,
    new_words: usize,
) -> *mut usize {
    let new_bytes = new_words * WORD_SIZE;
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
    let words = (bytes + WORD_SIZE - 1) / WORD_SIZE;
    dealloc_words(ptr as *mut usize, words);
}

// Helper for major allocation
pub unsafe fn alloc_major_short(
  tup_list: &mut *mut usize,
  words: usize,
  tag: u16,
) -> Tup {
  let ptr = alloc_words(
    Tup::MAJOR_EXTRA_WORDS + Tup::words_from_words(words)
  );
  ptr.write((*tup_list) as usize);
  let ptr = ptr.add(Tup::MAJOR_EXTRA_WORDS);
  *tup_list = ptr;
  ptr.write(Hd::short(words, tag).0);
  Tup(ptr)
}

pub unsafe fn alloc_major_long(tup_list: &mut *mut usize, bytes: usize) -> Tup {
  let ptr = alloc_words(
    Tup::MAJOR_EXTRA_WORDS + Tup::words_from_bytes(bytes)
  ) as *mut usize;
  ptr.write(*tup_list as usize);
  let ptr = ptr.add(Tup::MAJOR_EXTRA_WORDS);
  *tup_list = ptr;
  ptr.write(Hd::long(bytes).0);
  Tup(ptr)
}

pub unsafe fn dealloc_major_next(tup_list: *mut *mut usize) -> usize {
  let next = Tup(*tup_list);
  let next_next = next.major_next();
  tup_list.write(next_next.0);
  let words = Tup::words_from_words(next.header().words());
  dealloc_words(
    next.0.sub(Tup::MAJOR_EXTRA_WORDS),
    Tup::MAJOR_EXTRA_WORDS + words);
  words
}

/* Pool
 */

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
    unsafe {
      dealloc_words(self.ptr, self.words)
    }
  }
}

impl Pool {
  pub fn new(words: usize) -> Self {
    let ptr = unsafe { alloc_words(words) };
    Self {
      bytes: words * WORD_SIZE,
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

  #[inline(always)]
  pub fn alloc_unchecked(&mut self, words: usize) -> *mut usize {
    self.left -= words;
    unsafe { self.ptr.add(self.left as usize) }
  }

  #[inline(always)]
  pub fn allocatable_in_words(&self, words: usize) -> bool {
    self.left >= Tup::words_from_words(words)
  }

  #[inline(always)]
  pub fn allocatable_in_bytes(&self, bytes: usize) -> bool {
    self.left >= Tup::words_from_bytes(bytes)
  }
}

/* Mem & GC
 */

pub struct Mem {
  // Configurations
  pub major_gc_threshold_percent: usize,

  // Minor heap
  pub minor_pool: Pool,

  // Major heap
  pub major_allocated_words: usize,
  pub major_threshold_words: usize,
  
  // Major tuple list
  pub major_list: [*mut usize; 2],

  // Stack
  pub stack: Pool,

  // Global
  pub global: Pool,

  // Statistics
  pub minor_gc_count: usize,
  pub major_gc_count: usize,
  
  // GC template variables
  pub mark_list: Tup,
  pub write_barrier_list: Tup,
}

impl Drop for Mem {
  fn drop(&mut self) {
    for i in 0..Self::MAJOR_LIST_KINDS {
      let lst: *mut *mut usize = &mut self.major_list[i];
      unsafe {
        while !(*lst).is_null() {
            self.major_allocated_words -= dealloc_major_next(lst);
        }
      }
    }
  }
}

impl Mem {
  const MAJOR_LIST_LEAVES: usize = 0;
  const MAJOR_LIST_NODES: usize = 1;
  const MAJOR_LIST_KINDS: usize = 2;

  const MIN_MAJOR_SIZE_FACTOR: usize = 7;

  pub fn new(
    major_gc_threshold_percent: usize,
    minor_pool_size: usize,
    stack_size: usize,
    global_size: usize,
  ) -> Self {
    let mut mem = Self {
      // Configurations
      major_gc_threshold_percent: major_gc_threshold_percent,
      // Minor heap
      minor_pool: Pool::new(minor_pool_size),
      // Major heap
      major_allocated_words: 0,
      major_threshold_words: 0,
      // Major tuple list
      major_list: [std::ptr::null_mut(), std::ptr::null_mut()],
      // Stack
      stack: Pool::new(stack_size),
      // Global
      global: Pool::new(global_size),
      // Statistics
      minor_gc_count: 0,
      major_gc_count: 0,
      // GC template variables
      mark_list: Tup::NULL,
      write_barrier_list: Tup::NULL,
    };
    mem.update_major_gc_threshold();
    mem
  }

  pub fn update_major_gc_threshold(&mut self) {
    // If major_gc_threshold_percent is 0, disable major GC
    if self.major_gc_threshold_percent == 0 {
      self.major_threshold_words = usize::MAX;
      return;
    }
    // Calculate new major gc threshold percent
    let percent = self.major_gc_threshold_percent
      .checked_add(100)
      .unwrap_or(usize::MAX);
    // Set major gc threshold to minimum major heap size
    let mut new_size = self.minor_pool.words
      .checked_mul(Self::MIN_MAJOR_SIZE_FACTOR)
      .unwrap_or(usize::MAX);
    // To calculate percentage, split allocated into two part
    let lo = self.major_allocated_words % 100;
    let hi = self.major_allocated_words / 100;
    // Calculate fpercentage with checking overflow
    let hi_percented = hi.checked_mul(percent)
      .unwrap_or(usize::MAX);
    let lo_percented = lo.checked_mul(percent)
      .unwrap_or(usize::MAX);
    let percented = hi_percented.checked_add(lo_percented)
      .unwrap_or(usize::MAX);
    // Choose max one
    if percented > self.major_allocated_words {
      new_size = usize::MAX;
    }
    // Update size
    self.major_threshold_words = new_size;
  }

  #[inline(always)]
  fn alloc_major_long(&mut self, bytes: usize) -> Tup {
    unsafe {
      let ptr = alloc_major_long(
        &mut self.major_list[Self::MAJOR_LIST_LEAVES], bytes);
        let words = Tup::words_from_bytes(bytes);
      self.major_allocated_words += words + Tup::MAJOR_EXTRA_WORDS;
      ptr
    }
  }

  #[inline(always)]
  fn alloc_major_short(&mut self, words: usize, tag: u16) -> Tup {
    unsafe {
      let ptr = alloc_major_short(
        &mut self.major_list[Self::MAJOR_LIST_NODES],
        words,
        tag);
      self.major_allocated_words += words + Tup::MAJOR_EXTRA_WORDS;
      ptr
    }
  }

  #[inline(always)]
  fn alloc_minor_long(&mut self, bytes: usize) -> Tup {
    unsafe {
      let words = Tup::words_from_bytes(bytes);
      let a_words = words + Tup::MINOR_EXTRA_WORDS;
      let ptr = self.minor_pool.alloc_unchecked(a_words);
      let tup = Tup(ptr.add(Tup::MINOR_EXTRA_WORDS));
      tup.set_header(Hd::long(bytes));
      tup
    }
  }

  #[inline(always)]
  fn alloc_minor_short(&mut self, words: usize, tag: u16) -> Tup {
    unsafe {
      let a_words = Tup::words_from_words(words) + Tup::MINOR_EXTRA_WORDS;
      let ptr = self.minor_pool.alloc_unchecked(a_words);
      let tup = Tup(ptr.add(Tup::MINOR_EXTRA_WORDS));
      tup.set_header(Hd::short(words, tag));
      tup
    }
  }

  // GC

  fn mark_phase(&mut self) -> Result<(), ()> {
    unimplemented!()
  }

  fn mark_phase_major(&mut self) -> Result<(), ()> {
    unimplemented!()
  }

  fn mark_phase_minor(&mut self) -> Result<(), ()> {
    unimplemented!()
  }

  fn free_unmarked_major(&mut self) -> Result<(), ()> {
    unimplemented!()
  }

  fn move_minor_to_major(&mut self) -> Result<(), ()> {
    unimplemented!()
  }
  
  pub fn full_gc(&mut self) -> Result<(), ()> {
    // Run marking phase
    self.mark_phase_major()?;
    // Traverse object list and free unmarked objects
    // also unmark all marked objects
    self.free_unmarked_major()?;
    // Move marked object in minor heap to major heap
    self.move_minor_to_major()?;
    // Rewind minor heap's top pointer (left)
    self.minor_pool.rewind();
    // Adjust major gc threshold
    self.update_major_gc_threshold();
    // Clean write barrier
    self.write_barrier_list = Tup::NULL;
    // Update statistics
    self.major_gc_count += 1;
    Ok(())
  }

  pub fn minor_gc(&mut self) -> Result<(), ()> {
    // Check major heap is full
    let minor_allocated = self.minor_pool.words - self.minor_pool.left;
    let major_allocated_guess = self.major_allocated_words
      .checked_add(minor_allocated)
      .unwrap_or(usize::MAX);
    if major_allocated_guess > self.major_threshold_words {
      return self.full_gc();
    }
    // Run marking phase
    self.mark_phase_minor()?;
    // Move marked objects to major heap
    self.move_minor_to_major()?;
    // Rewind minor heap's top pointer (left)
    self.minor_pool.rewind();
    // Clean write barrier
    self.write_barrier_list = Tup::NULL;
    // Update statistics
    self.minor_gc_count += 1;
    Ok(())
  }

  // Allocations

  pub fn alloc_long(&mut self, bytes: usize) -> Tup {
    // Get size
    let size = Tup::words_from_bytes(bytes) + Tup::MINOR_EXTRA_WORDS;
    if size < self.minor_pool.left {
      return self.alloc_minor_long(bytes);
    } else if size > self.minor_pool.words {
      return self.alloc_major_long(bytes);
    } else {
      if let Err(()) = self.minor_gc() {
        panic!("minor gc failed")
      }
      return self.alloc_minor_long(bytes);
    }
  }
  
  pub fn alloc_short(&mut self, words: usize, tag: u16) -> Tup {
    // Get size
    let size = Tup::words_from_words(words) + Tup::MINOR_EXTRA_WORDS;
    if size < self.minor_pool.left {
      return self.alloc_minor_short(words, tag);
    } else if size > self.minor_pool.words {
      return self.alloc_major_short(words, tag);
    } else {
      if let Err(()) = self.minor_gc() {
        panic!("minor gc failed")
      }
      return self.alloc_minor_short(words, tag);
    }
  }

  // Write Barrier
  pub fn write_barrier(&mut self, tup: Tup) {
    // Check tup is unmarked
    if !tup.header().is_white() {
      return;
    }
    // If the object is white, mark it as gray
    tup.set_header(tup.header().marked(Hd::COLOR_GRAY));
    // Push tup to write barrier list
    unsafe {
      tup.set_gc_next(self.write_barrier_list);
      self.write_barrier_list = tup;
    }
  }
}